#include "uart.h"

#include "stm32f303xc.h"

struct _SerialPort {
	USART_TypeDef *UART;
	GPIO_TypeDef *GPIO;
	volatile uint32_t MaskAPB2ENR;	// mask to enable RCC APB2 bus registers
	volatile uint32_t MaskAPB1ENR;	// mask to enable RCC APB1 bus registers
	volatile uint32_t MaskAHBENR;	// mask to enable RCC AHB bus registers
	volatile uint32_t SerialPinModeValue;
	volatile uint32_t SerialPinSpeedValue;
	volatile uint32_t SerialPinAlternatePinValueLow;
	volatile uint32_t SerialPinAlternatePinValueHigh;
	void (*completion_function)(uint8_t*, uint32_t, uint32_t);
};

SerialPort USART1_PORT = {USART1,
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


// serialInitialise - Initialise the serial port
// Input: baudRate is from an enumerated set
void serialInitialise(SerialPort *serial_port, uint32_t baudRate, void (*completion_function)(uint8_t*, uint32_t, uint32_t)) {


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


	// enable serial port for tx and rx
	serial_port->UART->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void sendByte(SerialPort *serial_port, uint8_t data, void (*callback)()) {

	while((serial_port->UART->ISR & USART_ISR_TXE) == 0){
	}

	serial_port->UART->TDR = data;

	if (callback != 0x00) {
		callback();
	}
}


void sendString(SerialPort *serial_port, uint8_t *pt) {

	uint32_t counter = 0;
	while(*pt) {
		sendByte(serial_port, *pt, 0x00);
		counter++;
		pt++;
	}

}

void sendMsg(SerialPort *serial_port, uint8_t *buffer, size_t size, uint8_t message_id) {}

void receiveMsg(SerialPort *serial_port) {}


void simpleDelay() {
	// 0xA2C2B -> "666667 clock cycles"
	// from testing (using a metronome) this is ~1 second
	for (uint32_t i = 0; i < 0xA2C2B; ++i) {}
}

void testSerial() {
	serialInitialise(&USART1_PORT, BAUD_9600, 0x00);

	for (;;) {
		sendString(&USART1_PORT, (uint8_t*)"hello\r\n");
		simpleDelay();
	}
}
