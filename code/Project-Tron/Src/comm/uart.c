#include "comm/uart.h"

#include "stm32f303xc.h"
#include <string.h>
#include "timer.h"
#include "io/led.h"


#define MAX_UART_BUFFER 256
#define STX_CHARACTER 0x02
#define ETX_CHARACTER 0x03

#define FLASH_LED 0

// helper function to flash leds
void flashLeds() {
	if (!FLASH_LED) {
		return;
	}

	for (int i = 0; i < 2; ++i) {
		led_set_all(0xFF);
		delayElapsed(500);
		led_set_all(0x0);
		delayElapsed(500);
	}

}


void sendNextByte(SerialPort *serial_port);
void receiveNextByte(SerialPort *serial_port);

typedef struct ReceiverBufferMetadata {
	uint8_t receiveBuffer[MAX_UART_BUFFER];
	uint32_t receiveBufferSize;
	uint8_t receiveError;
} ReceiverBufferMetadata;

// note that switch buffer must be called by the kernel, since the kernel
// can't be interrupted by the user
typedef struct ReceiverDoubleBuffer {
	// true when kernel has started writing data
	uint8_t writeStarted;

	// when the kernel has finished processing data writeFinished == 1
	// kernel sets writeFinished to 0 or 1
	uint8_t writeFinished;


	// used to determine whether the reading is in progress
	// if user is currently reading, the kernel will not switch the buffer
	// this effectively acts like a mutex
	uint8_t readStarted;

	uint8_t readFinished;

	// kernel index specifies the index of the interrupt (i.e. "kernel")
	// index of the main progream (i.e. "user") is 1 - kernelIndex
	uint8_t kernelIndex;
	ReceiverBufferMetadata buffers[2];
} ReceiverDoubleBuffer;


typedef struct UARTMetadata {
	uint8_t* sendBuffer;
	uint32_t sendBufferIndex;
	uint32_t sendBufferSize;

	ReceiverDoubleBuffer doubleBuffer;
} UARTMetadata;



struct _SerialPort {
	UARTMetadata metadata;
	USART_TypeDef *UART;
	GPIO_TypeDef *GPIO;
	volatile uint32_t IRQn;
	volatile uint32_t MaskAPB2ENR;	// mask to enable RCC APB2 bus registers
	volatile uint32_t MaskAPB1ENR;	// mask to enable RCC APB1 bus registers
	volatile uint32_t MaskAHBENR;	// mask to enable RCC AHB bus registers
	volatile uint32_t SerialPinModeValue;
	volatile uint32_t SerialPinSpeedValue;
	volatile uint32_t SerialPinAlternatePinValueLow;
	volatile uint32_t SerialPinAlternatePinValueHigh;
	void (*completion_function)(uint8_t*, uint8_t, uint8_t);
};




void UARTIRQHandler(SerialPort* serial_port) {
	// if we have an overrun error
	// would be good to show error in the LED
	// note that we explicitly check for ORE since by enabling RXNEIE
	// either ORE or RXNE can trigger this interrupt
	if (serial_port->UART->ISR & USART_ISR_ORE) {
		// clear the ORE bit so it doesnt refire this interrupt
		serial_port->UART->ICR |=  USART_ICR_ORECF;
	}

	sendNextByte(serial_port);
	receiveNextByte(serial_port);
}

// NOTE USART1 uses
// PC4 => TX
// PC5 => RX
SerialPort USART1_PORT = {
		{0},
		USART1,
		GPIOC,
		USART1_IRQn,
		RCC_APB2ENR_USART1EN, // bit to enable for APB2 bus
		0x00,	// bit to enable for APB1 bus
		RCC_AHBENR_GPIOCEN, // bit to enable for AHB bus
		0xA00, // pin mode value (MODER4, MODER5) => Alternate Function
		0xF00, // pin speed value (OSPEEDR4, OSPEEDR5) => (high speed)
		0x770000,  // for USART1 PC4 and 5, this is in the AFR low register
		0x00, // no change to the high alternate function register
		0x00 // default function pointer is NULL
		};

// PC10 => UART4_TX
// PC11 => UART4_RX
SerialPort UART4_PORT = {
		{0},
		UART4,
		GPIOC,
		UART4_IRQn,
		0x00, // bit to enable for APB2 bus
		RCC_APB1ENR_UART4EN,	// bit to enable for APB1 bus
		RCC_AHBENR_GPIOCEN, // bit to enable for AHB bus
		0xA00000, // pin mode value (MODER10, MODER11) => Alternate Function
		0xF00000, // pin speed value (OSPEEDR10, OSPEEDR11) => (high speed)
		0x00,  // no change to the high alternate function register
		0x5500, // for UART4 PC10 and 11, this is in the AFR high register
		0x00 // default function pointer is NULL
};

// IRQ event handler
void USART1_EXTI25_IRQHandler() {
	UARTIRQHandler(&USART1_PORT);
}

void UART4_EXTI34_IRQHandler() {
	UARTIRQHandler(&UART4_PORT);
}


void sendNextByte(SerialPort *serial_port) {
	UARTMetadata* metadata = &serial_port->metadata;

	uint32_t index = metadata->sendBufferIndex;


	// check that the transmit register is ready
	// if not ready then don't send anything
	if (!(serial_port->UART->ISR & USART_ISR_TXE)) {
		return;
	}

	// if we have transmitted the entire buffer
	// reset the buffer and exit
	if (index >= metadata->sendBufferSize) {
		metadata->sendBufferIndex = 0;
		metadata->sendBufferSize = 0;
		metadata->sendBuffer = 0;

		// clear UART transmit interrupt
		// this prevents interrupt from refiring over and over
		serial_port->UART->CR1 &= ~USART_CR1_TXEIE;
		return;
	}

	// write to the transmit register
	serial_port->UART->TDR = metadata->sendBuffer[metadata->sendBufferIndex];

	// go to the next byte
	++metadata->sendBufferIndex;

}

void switchBuffer(ReceiverDoubleBuffer* doubleBuffer) {
	// switch the buffer when both the kernel and the user is not reading the buffer
	// and the kernel has finished writing to the buffer
	// this way the user will always get the latest data
	if ((doubleBuffer->readFinished || !doubleBuffer->readStarted)  && doubleBuffer->writeFinished) {
		doubleBuffer->writeStarted = 0;
		doubleBuffer->writeFinished = 0;
		doubleBuffer->readStarted = 0;
		doubleBuffer->readFinished = 0;

		doubleBuffer->kernelIndex = 1 - doubleBuffer->kernelIndex;
	}
}

// ONLY KERNEL SHOULD CALL THIS
// SIGNALS THE KERNEL IS NOW WRITING TO THE BUFFER
// SWITCHES THE DOUBLE BUFFER IF USER HAS FINISHED READING THE BUFFER
// IF THE USER STILL HASN'T READ IT IT OVERWRITES THE BUFFER
void startBufferWrite(ReceiverDoubleBuffer* doubleBuffer) {
	switchBuffer(doubleBuffer);

	doubleBuffer->writeStarted = 1;
	doubleBuffer->writeFinished = 0;


}

// GETS THE BUFFER
ReceiverBufferMetadata* getBuffer(ReceiverDoubleBuffer* doubleBuffer, uint8_t isKernel) {
	if (isKernel) {
		return doubleBuffer->buffers + doubleBuffer->kernelIndex;
	}
	return doubleBuffer->buffers + (1 - doubleBuffer->kernelIndex);
}


void startBufferRead(ReceiverDoubleBuffer* doubleBuffer) {
	doubleBuffer->readStarted = 1;
}

// SIGNALS THAT THE USER HAS FINISHED READING THE BUFFER
void finishBufferRead(ReceiverDoubleBuffer* doubleBuffer) {
	doubleBuffer->readFinished = 1;
}

// SIGNALS THE KERNEL HAS FINISHED WRITING TO THE BUFFER
// SWITCHES THE DOUBLE BUFFER IF USER HAS FINISHED READING THE BUFFER
void finishBufferWrite(ReceiverDoubleBuffer* doubleBuffer) {
	doubleBuffer->writeFinished = 1;

	// switch the buffer when both the kernel and the user has finished reading
	// and writing
	switchBuffer(doubleBuffer);

}


void receiveNextByte(SerialPort *serial_port) {
	UARTMetadata* metadataAll = &serial_port->metadata;
	ReceiverDoubleBuffer* doubleBuffer = &metadataAll->doubleBuffer;

	// check that the receive register has something
	// if not ready then return
	if (!(serial_port->UART->ISR & USART_ISR_RXNE)) {
		return;
	}


	// read the buffer as the kernel
	ReceiverBufferMetadata* metadata = getBuffer(doubleBuffer, 1);

	// now read from the buffer, to prevent the interrupt from refiring
	uint8_t ch =  (uint8_t) serial_port->UART->RDR;


	// if the 1st charac is STX
	// if the write hasn't started STX is first character
	// if the write has finished STX is also the first character
	// in this case the buffer always has the latest data
	if (ch == STX_CHARACTER && (!doubleBuffer->writeStarted || doubleBuffer->writeFinished)) {
		// if buffer has been written, but there's a new incoming message
		// set the buffer to not write finished, and replace the buffer with the new message
		startBufferWrite(doubleBuffer);
		metadata = getBuffer(doubleBuffer, 1);

		metadata->receiveBufferSize = 0;
		// clear the receive error
		metadata->receiveError = 0;
	}

	// if STX bit is not seen, and the write hasn't started do nothing
	if (!doubleBuffer->writeStarted) {
		 return;
	}

	// if the write has finished try switching the buffer
	// finishBufferWrite attempts to switch the buffer
	if (doubleBuffer->writeFinished) {
		return finishBufferWrite(doubleBuffer);
	}




	// if we reach max size of the buffer, we're done
	// ERROR
	if (metadata->receiveBufferSize >= MAX_UART_BUFFER) {
		metadata->receiveBufferSize = 0;
		metadata->receiveError = 1;
		return finishBufferWrite(doubleBuffer);
	}


	uint8_t claimedSize = 0;
	if (metadata->receiveBufferSize >= 2) {
		claimedSize = metadata->receiveBuffer[1];
	}


	// if its bigger than the claimed size stop and ERROR
	if (metadata->receiveBufferSize >= claimedSize && claimedSize != 0) {
		metadata->receiveError = 1;
		metadata->receiveBufferSize = 0;
		return finishBufferWrite(doubleBuffer);
	}


	if (metadata->receiveBufferSize >= 12) {
		metadata->receiveError = 0;
	}
	// note that the second condition is necessary since 0x03 could appear
	// in message id, the checksum or message length
	if (metadata->receiveBuffer[metadata->receiveBufferSize - 1] == ETX_CHARACTER
			&& metadata->receiveBufferSize + 1 == claimedSize) {

		// copy the checksum over to the buffer
		uint32_t size = metadata->receiveBufferSize;
		metadata->receiveBuffer[size] = ch;
		metadata->receiveBufferSize += 1;

		// finish writing to the buffer
		return finishBufferWrite(doubleBuffer);
	}

	// copy the character over to the buffer
	uint32_t size = metadata->receiveBufferSize;
	metadata->receiveBuffer[size] = ch;
	metadata->receiveBufferSize += 1;

}



void enableSerialInterrupt(SerialPort *serial_port) {
	__disable_irq();

	// enable receive buffer checking
	// we want to continuously receive data from the UART RX port
	// even as we're running the main program normally
	serial_port->UART->CR1 |= USART_CR1_RXNEIE;

	NVIC_EnableIRQ(serial_port->IRQn);

	__enable_irq();
}

// serialInitialise - Initialise the serial port
// Input: baudRate is from an enumerated set
void serialInitialise(SerialPort *serial_port, uint32_t baudRate, void (*completion_function)(uint8_t*, uint8_t, uint8_t)) {


	// enable clock power, system configuration clock and GPIOC
	// common to all UARTs
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

	// enable the GPIO which is on the AHB bus
	RCC->AHBENR |= serial_port->MaskAHBENR;

	// set pin mode to alternate function for the specific GPIO pins
	serial_port->GPIO->MODER |= serial_port->SerialPinModeValue;

	// enable high speed clock for specific GPIO pins
	serial_port->GPIO->OSPEEDR |= serial_port->SerialPinSpeedValue;

	// set alternate function to enable USART to external pins
	serial_port->GPIO->AFR[0] |= serial_port->SerialPinAlternatePinValueLow;
	serial_port->GPIO->AFR[1] |= serial_port->SerialPinAlternatePinValueHigh;

	// enable the device based on the bits defined in the serial port definition
	RCC->APB1ENR |= serial_port->MaskAPB1ENR;
	RCC->APB2ENR |= serial_port->MaskAPB2ENR;

	// Get a pointer to the 16 bits of the BRR register that we want to change
	uint16_t *baud_rate_config = (uint16_t*)&serial_port->UART->BRR; // only 16 bits used!

	// Baud rate calculation from datasheet
	// Baud = fck/USARTDIV, USARTDIV = fck/Baud (oversampling by 16, OVER8 = 0)
	switch(baudRate){
	case BAUD_9600:
		// 8 * 10^6 /(9600) = 0x341
		*baud_rate_config = 0x341;  // 9600 at 8MHz
		break;
	case BAUD_19200:
		// 8 * 10^6 /(19200) = 0x1A1
		*baud_rate_config = 0x1A1;  // 19200 at 8MHz
		break;
	case BAUD_38400:
		// 8 * 10^6 /(38400) = 0xD0
		*baud_rate_config = 0xD0;  // 38400 at 8MHz
		break;
	case BAUD_57600:
		// 8 * 10^6 /(57600) = 0x8B
		*baud_rate_config = 0x8B;  // 57600 at 8MHz
		break;
	case BAUD_115200:
		// 8 * 10^6 /(57600) = 0x45
		*baud_rate_config = 0x45;  // 115200 at 8MHz
		break;
	}

	enableSerialInterrupt(serial_port);

	// enable serial port for tx and rx
	serial_port->UART->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

	serial_port->completion_function = completion_function;

	// ensures that the reader spins in the beginning
	// while waiting for the writer to write to the buffer
	serial_port->metadata.doubleBuffer.readFinished = 1;
	serial_port->metadata.doubleBuffer.readStarted = 1;
}


// sends an arbitrary buffer asynchronously
// if the previous buffer hasn't been sent, it will spin until the previous buffer has been sent
void sendBuffer(SerialPort *serial_port, uint8_t* data, size_t size) {

	// the send buffer size becomes 0 only when the buffer has successfully sent the previous buffer
	// if it's not 0, we haven't finished transmitting the previous buffer, so we spin
	while (serial_port->metadata.sendBufferSize != 0) {}

	serial_port->metadata.sendBuffer = data;
	serial_port->metadata.sendBufferSize = size;
	serial_port->metadata.sendBufferIndex = 0;

	// enable UART transmit interrupt
	serial_port->UART->CR1 |= USART_CR1_TXEIE;

}


void sendString(SerialPort *serial_port, char* string) {

	size_t length = strlen(string);
	sendBuffer(serial_port, (uint8_t*) string, length);

}


uint8_t bcc_checksum(uint8_t* buffer, uint8_t size) {
	uint8_t bcc = 0;
	for (int i = 0; i < size; ++i) {
		bcc ^= buffer[i];
	}
	return bcc;
}

// Format: STX <message size> <message id> <buffer> ETX <checksum>
// STX is 0x02, ETX is 0x03, the checksum used is BCC 8-bit XOR checksum
void sendMsg(SerialPort *serial_port, uint8_t *buffer, uint8_t size, uint8_t message_id) {
	uint8_t uartBuffer[MAX_UART_BUFFER];
	uartBuffer[0] = STX_CHARACTER;

	// STX (1 byte), id (1 byte), size (1 byte), ETX (1 byte), checksum (1 byte)
	// additional bytes = 5 bytes
	uartBuffer[1] = size + 5;
	uartBuffer[2] = message_id;

	for (int i = 3; i < size + 3; ++i) {
		uartBuffer[i] = buffer[i-3];
	}

	uartBuffer[size + 3] = ETX_CHARACTER;
	uartBuffer[size + 4] = bcc_checksum(uartBuffer, size + 4);


	sendBuffer(serial_port, uartBuffer, size + 5);


}

void receiveMsg(SerialPort *serial_port) {
	UARTMetadata* metadataAll = &serial_port->metadata;
	ReceiverDoubleBuffer* doubleBuffer = &metadataAll->doubleBuffer;

	// keep looping until there's data available for the user
	// when data is available, readFinished will be reset to 0
	while (doubleBuffer->readFinished == 1) {};

	// let's say it interrupts here
	// then it switches the buffer, this is OKAY since everytime it switches
	// its guaranteed that the received message is valid and ready to be read

	startBufferRead(doubleBuffer);

	// past here, any attempts to switch the buffer will be blocked
	// since startBufferRead puts "read in progress"

	// read the buffer as a user
	ReceiverBufferMetadata* metadata = getBuffer(doubleBuffer, 0);

	// parse the message
	uint8_t* buffer = metadata->receiveBuffer;
	uint8_t size = metadata->receiveBufferSize - 5;

	// VALIDATION CHECKS:

	// check if receive error encountered
	if (metadata->receiveError) {
		flashLeds();
		return finishBufferRead(doubleBuffer);
	}

	// validation check: check if the buffer length is as claimed
	if (size + 5 != buffer[1]) {
		flashLeds();
		return finishBufferRead(doubleBuffer);
	}

	// check the checksum is valid
	if (bcc_checksum(buffer, size + 4) != buffer[size+4]) {
		flashLeds();
		return finishBufferRead(doubleBuffer);
	}

	// copy the buffer, since our interrupt might overwrite our old buffer
	uint8_t newBuffer[MAX_UART_BUFFER];
	for (int i = 3; i < size + 3; ++i) {
		newBuffer[i-3] = buffer[i];
	}

	uint8_t message_id = buffer[2];

	finishBufferRead(doubleBuffer);

	// call the completion function passed in
	if (serial_port->completion_function != 0x00) {
		serial_port->completion_function(newBuffer, size, message_id);
	}

}




/*

Q1
How do you handle the case when there is more data received than will fit in the
buffer? => Line 291
if (metadata->receiveBufferSize >= MAX_UART_BUFFER) {
		metadata->receiveBufferSize = 0;
		metadata->receiveError = 1;
		return finishBufferWrite(doubleBuffer);
	}
=> receiveError will be set to 1, and buffer writing will terminate
=> when buffer writing terminates reader can then read from the buffer
=> receiveError, set to 1, (Line 506) which causes LEDs to flash if FLASH_LED define is set to 1

Q2
How do you determine when the incoming data has finished being received? Do
you use a terminating character? What if this byte is missed of if the same byte
appears elsewhere in the received data?
=> ETX bit will be received, indicates termination
=> if same byte appears elsewhere, ignore since we also account for the length of the buffer
=> see line 317 (check ETX AND check if ETX is in the right position (BUFFER_LENGTH - 1))

Q3
What are some potential advantages and disadvantages of passing structures and
raw bytes rather than strings as we did in the Assembly lab?
=> Data needs to be converted to strings (and parsed back), this is slow!
=> Also ASCII strings are only 7 bits long very inefficient!

=> why strings? if the data is already a string, its better to just send the string
=> no need to keep track of data length since string is null terminating

Q4
How do other software modules interact with the received data? Can they request
the latest data? What happens with the incoming memory buffer after someone
has requested the data? Do you clear the buffer? Do you have more than one
buffer?.
=> First initialize, then call receiveMessage().
=> This requests for the data and if the data is available, callback is called
=> if not then it spins

=> Latest data is always received:
=> Kernel receives data from UART, because of double buffer,
=> new data is continually written into the kernel's buffer
=> Kernel swaps when reader is finished reading

=> Incoming memory buffer is left alone,
=> until the kernel swaps the double buffer and overwrites it
=> to prevent race conditions kernel only swaps if the reader is not reading the buffer

Q5
What happens if someone requests new serial port data before the current stream
of data is complete (i.e. before the stream is terminated)?
=> if reader buffer is empty, receiveMessage() will spin until kernel swaps
=> if reader buffer is not empty, then it just reads the buffer

Q6
What if someone keeps working on the serial rx buffer at the same time as new
data comes in?
=> Buffer is copied to a new buffer, so the caller won't experience anything
=> when reader is reading the buffer inside of receiveMsg, kernel won't swap
=> unless finishedReading is set to 1, hence buffer remains consistent


Advanced Demo:
Set delay to 3000
Breakpoint in receiveMessage:
Play:
Send the following within 3 seconds
echo -ne '\x02\x0d\x02hello\r\n\0\x03k' > /dev/ttyACM0
echo -ne '\x02\x0d\x02hello\r\n\0\x03k' > /dev/ttyACM0
echo -ne '\x02\x0d\x02' > /dev/ttyACM0


At the next receive, double buffer will look like
Kernel: '\x02\x0d\x02'
Reader: '\x02\x0d\x02hello\r\n\0\x03k'

Press Play, notice that the reader can read the buffer
While the kernel is "reading" the serial port

Then next iteration, reader will again pause in receiveMessage()
Finally,
echo -ne 'hello\r\n\0\x03k' > /dev/ttyACM0
Then reader can read the rest of the message

 */
uint32_t counter = 0;
void serialCallback(uint8_t* buffer, uint8_t size, uint8_t id) {
	// empty function, put a breakpoint here to see this working
	counter += 1;
	led_set_all(counter);
}

void testSerial() {
	serialInitialise(&USART1_PORT, BAUD_9600, serialCallback);
	initElapsedTimer();
	led_init();

	for (;;) {
		// port, buffer, length of buffer, message id (good to define message id enum somewhere)
		sendMsg(&USART1_PORT, (uint8_t*) "hello\r\n", 8, 2);
		receiveMsg(&USART1_PORT);
		delayElapsed(1000);
	}


}

void testSerialString() {
	// pass in 0x00 NULL callback
	serialInitialise(&USART1_PORT, BAUD_9600, 0x00);
	initElapsedTimer();
	led_init();

	for (;;) {
		// sends "hello" through UART port
		sendString(&USART1_PORT, "hello\r\n");
		delayElapsed(1000);
	}
}
