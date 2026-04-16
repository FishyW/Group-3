// integration file for board 1
#include "integration.h"


void initializeBoardA() {

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

	while (1) {
		// Jack
		// magData = READ FROM I2C
		// read button toggle state

		// Winston
		// pass <magData + button state> to UART

		// Denny
		// delay(DELAY_TIME_MILLISECONDS)
		// timer_oneshot_call(user_delay, user_delay_cb());
	}
}
