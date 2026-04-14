#include "gyro.h"

#include "gpio.h"

void initializeGyro() {

	// first set CS (Chip select) to 0
	// Chip Select is PE3
	gpio_t gpioCS;
	gpio_init(&gpioCS, GPIOE, 3, GPIO_MODE_OUTPUT);
	gpio_write(&gpioCS, 0);

	// configure GPIO for MOSI, MISO and SCK
	// enable GPIO clock for port A
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	// GPIOA->MODER
}


void testGyro() {
	initializeGyro();
}
