// integration file for board 2
#include "integration.h"

void initializeBoardB() {

}

// Let there be 2 boards: Board A and Board B
// Board A sends over UART, magnetometer data to Board B
// Board A needs interrupt driven function, respond to a button
// send over UART how to display output

// Board B wait to receive messages
// Board B takes magnetometer data, change position of the servo
// Board B takes button data
// button state => 1, use LED array else use servo
void runBoardB() {
	initializeBoardB();

	while (1) {
			uint8_t buttonState = 0;
			// Winston
			// read and parse mag/button state data

			if (buttonState == 1) {
				// Jack
				// change LED state based on magnetometer
			} else {
				// Denny
				// change servo state based on magentometer
			}


		}
}
