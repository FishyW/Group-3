/**
 * TO DO: If RX receives overrun error, display error in the LED
 * TO DO: If message buffer is invalid, display error in LED
 * TO DO: Replace completion function with LED display
 */

#include "comm/uart.h"

#include "stm32f303xc.h"
#include <string.h>


#define MAX_UART_BUFFER 256
#define STX_CHARACTER 0x02
#define ETX_CHARACTER 0x03

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
	// when the user has finished reading the data readFinished == 1
	// kernel sets readFinished to 0, user sets readFinished to 1
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
	volatile uint32_t MaskAPB2ENR;	// mask to enable RCC APB2 bus registers
	volatile uint32_t MaskAPB1ENR;	// mask to enable RCC APB1 bus registers
	volatile uint32_t MaskAHBENR;	// mask to enable RCC AHB bus registers
	volatile uint32_t SerialPinModeValue;
	volatile uint32_t SerialPinSpeedValue;
	volatile uint32_t SerialPinAlternatePinValueLow;
	volatile uint32_t SerialPinAlternatePinValueHigh;
	void (*completion_function)(uint8_t*, uint8_t, uint8_t);
};


SerialPort USART1_PORT = {
		{0},
		USART1,
		GPIOC,
		RCC_APB2ENR_USART1EN, // bit to enable for APB2 bus
		0x00,	// bit to enable for APB1 bus
		RCC_AHBENR_GPIOCEN, // bit to enable for AHB bus
		0xA00,
		0xF00,
		0x770000,  // for USART1 PC10 and 11, this is in the AFR low register
		0x00, // no change to the high alternate function register
		0x00 // default function pointer is NULL
		};




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
	// switch the buffer when both the kernel and the user has finished reading
	// and writing
	if (doubleBuffer->readFinished && doubleBuffer->writeFinished) {
		doubleBuffer->readFinished = 0;
		doubleBuffer->writeStarted = 0;
		doubleBuffer->writeFinished = 0;

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


	// if the 1st charac is STX (i.e. buffer
	if (ch == STX_CHARACTER && !doubleBuffer->writeStarted) {
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


// IRQ event handler
void USART1_EXTI25_IRQHandler() {

	// if we have an overrun error
	// would be good to show error in the LED
	// note that we explicitly check for ORE since by enabling RXNEIE
	// either ORE or RXNE can trigger this interrupt
	if (USART1_PORT.UART->ISR & USART_ISR_ORE) {
		// clear the ORE bit so it doesnt refire this interrupt
		USART1_PORT.UART->ICR |=  USART_ICR_ORECF;
	}

	sendNextByte(&USART1_PORT);
	receiveNextByte(&USART1_PORT);
}

void enableSerialInterrupt(SerialPort *serial_port) {
	__disable_irq();

	// enable receive buffer checking
	// we want to continuously receive data from the UART RX port
	// even as we're running the main program normally
	serial_port->UART->CR1 |= USART_CR1_RXNEIE;

	NVIC_EnableIRQ(USART1_IRQn);

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

	// set read finished to 1, since if read finished is 1, the user will spin
	serial_port->metadata.doubleBuffer.readFinished = 1;
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

	// if nothing is in the buffer, buffer is in state "read finished"
	// once the kernel has switched the buffer the state transitions to read not finished
	// in which case we read it
	while (doubleBuffer->readFinished == 1) {};

	// read the buffer as a user
	ReceiverBufferMetadata* metadata = getBuffer(doubleBuffer, 0);

	// parse the message
	uint8_t* buffer = metadata->receiveBuffer;
	uint8_t size = metadata->receiveBufferSize - 5;

	// VALIDATION CHECKS:
	// set LED when validation checks fail

	// check if receive error encountered
	if (metadata->receiveError) {
		return finishBufferRead(doubleBuffer);
	}

	// validation check: check if the buffer length is as claimed
	if (size + 5 != buffer[1]) {
		return finishBufferRead(doubleBuffer);
	}

	// check the checksum is valid
	if (bcc_checksum(buffer, size + 4) != buffer[size+4]) {
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


void simpleDelay() {
	// 0xA2C2B -> "666667 clock cycles"
	// from testing (using a metronome) this is ~1 second
	for (uint32_t i = 0; i < 0xA2C2B; ++i) {}
}

void serialCallback(uint8_t* buffer, uint8_t size, uint8_t id) {
	// empty function, put a breakpoint here to see this working
}

void testSerial() {
	serialInitialise(&USART1_PORT, BAUD_9600, serialCallback);

	for (;;) {
		// port, buffer, length of buffer, message id (good to define message id enum somewhere)
		sendMsg(&USART1_PORT, (uint8_t*) "hello\r\n", 8, 2);
		receiveMsg(&USART1_PORT);
		simpleDelay();
	}
}

void testSerialString() {
	// pass in 0x00 NULL callback
	serialInitialise(&USART1_PORT, BAUD_9600, 0x00);

	for (;;) {
		// sends "hello" through UART port
		sendString(&USART1_PORT, "hello\r\n");
		simpleDelay();
	}
}
