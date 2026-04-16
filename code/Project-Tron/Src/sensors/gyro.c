#include <stdio.h>

#include "sensors/gyro.h"

#include "io/gpio.h"
#include "comm/uart.h"

#define GYRO_WHO_AM_I_REG 0xF
#define GYRO_CTRL_REG1 0x20
#define GYRO_STATUS_REG 0x27
#define GYRO_OUT_X_L 0x28
#define GYRO_OUT_X_H 0x29
#define GYRO_OUT_Y_L 0x2A
#define GYRO_OUT_Y_H 0x2B
#define GYRO_OUT_Z_L 0x2C
#define GYRO_OUT_Z_H 0x2D

gpio_t gpioCS;
void initializeGyroGPIO() {
	// first set CS (Chip select) to 0
	// Chip Select is PE3

	gpio_init(&gpioCS, GPIOE, 3, GPIO_MODE_OUTPUT);
	gpio_write(&gpioCS, 1);

	// configure GPIO for MOSI, MISO and SCK
	// enable GPIO clock for port A
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	// SPI1_SCK (SCL) => PA5, AF5
	GPIOA->MODER |= 2 << GPIO_MODER_MODER5_Pos; // "2: AF Mode"
	GPIOA->OSPEEDR |= 3 << GPIO_OSPEEDER_OSPEEDR5_Pos; // "3: High Speed mode
	// AFR Low
	GPIOA->AFR[0] |= 5 << GPIO_AFRL_AFRL5_Pos;

	// SPI1_MISO (SDO) => PA6, AF5
	GPIOA->MODER |= 2 << GPIO_MODER_MODER6_Pos; // "2: AF Mode"
	GPIOA->OSPEEDR |= 3 << GPIO_OSPEEDER_OSPEEDR6_Pos; // "3: High Speed mode
	// AFR Low
	GPIOA->AFR[0] |= 5 << GPIO_AFRL_AFRL6_Pos;

	// SPI1_MOSI (SDI) => PA7, AF5
	GPIOA->MODER |= 2 << GPIO_MODER_MODER7_Pos; // "2: AF Mode"
	GPIOA->OSPEEDR |= 3 << GPIO_OSPEEDER_OSPEEDR7_Pos; // "3: High Speed mode
	// AFR Low
	GPIOA->AFR[0] |= 5 << GPIO_AFRL_AFRL7_Pos;
}

void initializeGyro() {
	// enable SPI1 CLK
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

	initializeGyroGPIO();


	// write to SPI_CR1 register
	// Baud Rate set to PCLK/256 = 8MHz/256 = 31250
	// CPOL and CPHA set to 1, clock default HIGH, read at second edge (rising edge)
	// frame format MSb first, full duplex mode -> LSBFIRST and BIDIMODE = 0
	// MSTR => 1 (since we configure this to be master)
	SPI1->CR1 |= 0x7 << SPI_CR1_BR_Pos
				| SPI_CR1_CPOL | SPI_CR1_CPHA | SPI_CR1_MSTR;


	// set DS to 1111 => 16 bits data frame
	// set to SSOE mode (disables multimaster capability)
	SPI1->CR2 |= 0xF << SPI_CR2_DS_Pos
				| SPI_CR2_SSOE;

	// finally enable SPI
	SPI1->CR1 |= SPI_CR1_SPE;

}


void sendSPI(uint16_t message) {
	// SPI1->SR & SPI_SR_TXE => 0
	// means transmit buffer not empty
	// => keep looping
	while (!(SPI1->SR & SPI_SR_TXE)) {}


	// once transmit buffer is empty
	SPI1->DR = message;
}

uint8_t receiveSPI() {
	// SPI1->SR & SPI_SR_RXNE => 0
	// means receive buffer empty
	// => keep looping, wait until something populates the receive buffer
	while (!(SPI1->SR & SPI_SR_RXNE)) {}

	uint16_t message = SPI1->DR;
	return (uint8_t) message;
}

void simpleDelay2() {
	// 0xA2C2B -> "666667 clock cycles"
	// from testing (using a metronome) this is ~1 second
	for (uint32_t i = 0; i < 0x51615; ++i) {}
}

uint8_t readGyroRegister(uint8_t address) {
	uint16_t message = ((1 << 7) | address) << 8;

	gpio_write(&gpioCS, 0);
	sendSPI(message);
	uint8_t value = receiveSPI();
	gpio_write(&gpioCS, 1);

	return value;
}

void writeGyroRegister(uint8_t address, uint8_t value) {
	gpio_write(&gpioCS, 0);
	uint8_t message = address;
	sendSPI(message << 8 | value);
	receiveSPI();
	gpio_write(&gpioCS, 1);
}


void testGyro() {
	initializeGyro();
	serialInitialise(&USART1_PORT, BAUD_9600, 0x00);


	// turn on power, and Zen, Xen, Yen
	writeGyroRegister(GYRO_CTRL_REG1, 0b1111);



	while (1) {
		while (1) {
			uint8_t status = readGyroRegister(GYRO_STATUS_REG);
			if (status & (1 << 3)) {
				break;
			}
		}


		int16_t gyroX = 0, gyroY = 0, gyroZ = 0;
		gyroX |= readGyroRegister(GYRO_OUT_X_L);
		gyroX |= readGyroRegister(GYRO_OUT_X_H) << 8;

		gyroY |= readGyroRegister(GYRO_OUT_Y_L);
		gyroY |= readGyroRegister(GYRO_OUT_Y_H) << 8;

		gyroZ |= readGyroRegister(GYRO_OUT_Z_L);
		gyroZ |= readGyroRegister(GYRO_OUT_Z_H) << 8;

		char string[50];

		snprintf(string, sizeof(string), "%d %d %d\r\n", gyroX, gyroY, gyroZ);
		sendString(&USART1_PORT, string);
		simpleDelay2();
	}

}
