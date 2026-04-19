// integration file for board 1
#include "integration.h"

#include "comm/i2c.h"
#include "comm/uart.h"

#include "timer.h"


void initializeBoardA() {
	i2c1Init();
	serialInitialise(&USART1_PORT, BAUD_9600, 0x00);
	initElapsedTimer();
	magInit();
}

// Let there be 2 boards: Board A and Board B
// Board A sends over UART, magnetometer data to Board B
// Board A needs interrupt driven function, respond to a button
// send over UART how to display output

// Board B wait to receive messages
// Board B takes magnetometer data, change position of the servo
// Board B takes button data
// button state => 1, use LED array else use servo
void runBoardA() {
	initializeBoardA();
	BoardMessage boardMsg;

	while (1) {

		// Jack
		// magData = READ FROM I2C
		 magReadSample(&boardMsg.magSample);

		// read button toggle state

		// Winston
		// pass <magData + button state> to UART
		 sendMsg(&USART1_PORT, (uint8_t*) &boardMsg, sizeof(BoardMessage), 0);

		// set the delay to 10ms => 100Hz
		 // which is the same as the magnetometer ODR
		 delayElapsed(10);
	}
}
