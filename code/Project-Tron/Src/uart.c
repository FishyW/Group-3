/**
 * TO DO: If RX receives overrun error, display error in the LED
 * TO DO: If message buffer is invalid, display error in LED
 * TO DO: Replace completion function with LED display
 */

#include "uart.h"

#include "stm32f303xc.h"
#include <string.h>

#define DEFAULT_UART_METADATA {0,0,0,{},0,0,0}

#define MAX_UART_BUFFER 256
#define STX_CHARACTER 0x02
#define ETX_CHARACTER 0x03

typedef struct UARTMetadata {
	uint8_t* sendBuffer;
	uint32_t sendBufferIndex;
	uint32_t sendBufferSize;

	uint8_t receiveBuffer[MAX_UART_BUFFER];
	uint32_t receiveBufferSize;
	uint8_t receiveInProgress;
	uint8_t receiveError;
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
		DEFAULT_UART_METADATA,
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

void receiveNextByte(SerialPort *serial_port) {
	UARTMetadata* metadata = &serial_port->metadata;

	// check that the receive register has something
	// if not ready then return
	if (!(serial_port->UART->ISR & USART_ISR_RXNE)) {
		return;
	}

	// now read from the buffer, to prevent the interrupt from refiring
	uint8_t ch =  (uint8_t) serial_port->UART->RDR;

	// we need the second AND otherwise size of 0x02 or message id of 0x02
	// or checksum == 0x02 leads to the buffer getting reset
	if (ch == STX_CHARACTER && metadata->receiveInProgress == 0) {
		metadata->receiveInProgress = 1;
		metadata->receiveBufferSize = 0;
		// clear the receive error
		metadata->receiveError = 0;
	}

	// if STX bit not seen, then do nothing
	if (metadata->receiveInProgress == 0) {
		return;
	}


	// if we reach max size of the buffer, we're done
	// ERROR
	if (metadata->receiveBufferSize >= MAX_UART_BUFFER) {
		metadata->receiveInProgress = 0;
		metadata->receiveBufferSize = 0;
		metadata->receiveError = 1;
		return;
	}

	uint8_t claimedSize = 0;
	if (metadata->receiveBufferSize >= 2) {
		claimedSize = metadata->receiveBuffer[1];
	}


	// if its bigger than the claimed size stop and ERROR
	if (metadata->receiveBufferSize > claimedSize && claimedSize != 0) {
		metadata->receiveInProgress = 0;
		metadata->receiveError = 1;
		metadata->receiveBufferSize = 0;
		return;
	}

	if (metadata->receiveBufferSize == 12) {
		metadata->receiveError = 0;
	}
	// note that the second condition is necessary since 0x03 could appear
	// in message id, the checksum or message length
	if (metadata->receiveBuffer[metadata->receiveBufferSize - 1] == ETX_CHARACTER
			&& metadata->receiveBufferSize + 1 == claimedSize) {
		metadata->receiveInProgress = 0;
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
	serial_port->GPIO->MODER = serial_port->SerialPinModeValue;

	// enable high speed clock for specific GPIO pins
	serial_port->GPIO->OSPEEDR = serial_port->SerialPinSpeedValue;

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
	uint8_t uart_buffer[MAX_UART_BUFFER];
	uart_buffer[0] = STX_CHARACTER;

	// STX (1 byte), id (1 byte), size (1 byte), ETX (1 byte), checksum (1 byte)
	// additional bytes = 5 bytes
	uart_buffer[1] = size + 5;
	uart_buffer[2] = message_id;

	for (int i = 3; i < size + 3; ++i) {
		uart_buffer[i] = buffer[i-3];
	}

	uart_buffer[size + 3] = ETX_CHARACTER;
	uart_buffer[size + 4] = bcc_checksum(uart_buffer, size + 4);


	sendBuffer(serial_port, uart_buffer, size + 5);


}

void receiveMsg(SerialPort *serial_port) {
	UARTMetadata *metadata = &serial_port->metadata;
	// if ISR is still receiving data or if nothing is in the buffer
	// wait until something is in the buffer
	while (1) {
		// for now we'll put these to prevent race conditions
		// ideally we should implement double buffers
		__disable_irq();
		if (!metadata->receiveInProgress && metadata->receiveBufferSize != 0) {
			break;
		}
		__enable_irq();
	};


	// parse the message
	uint8_t* buffer = metadata->receiveBuffer;
	uint8_t size = metadata->receiveBufferSize - 5;

	// reset the buffer
	metadata->receiveBufferSize = 0;

	// VALIDATION CHECKS:
	// set LED when validation checks fail

	// check if receive error encountered
	if (metadata->receiveError) {
		metadata->receiveError = 0;
		return;
	}

	// validation check: check if the buffer length is as claimed
	if (size + 5 != buffer[1]) {
		return;
	}

	// check the checksum is valid
	if (bcc_checksum(buffer, size + 4) != buffer[size+4]) {
		return;
	}

	// copy the buffer, since our interrupt might overwrite our old buffer
	uint8_t new_buffer[MAX_UART_BUFFER];
	for (int i = 3; i < size + 3; ++i) {
		new_buffer[i-3] = buffer[i];
	}

	uint8_t message_id = buffer[2];
	__enable_irq();
	// call the completion function passed in
	if (serial_port->completion_function != 0x00) {
		serial_port->completion_function(new_buffer, size, message_id);
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
