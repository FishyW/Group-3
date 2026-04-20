// integration file for board 2
#include "integration.h"
#include <stdint.h>
#include <stdio.h>

#include "comm/uart.h"

#include "servo.h"
#include "timer.h"

#include "servo.h"

#include "io/led.h"

#include "math.h"

void writeHeadingToLEDs(float heading) {
	// NORTH
	if (heading >= -22.5 && heading < 22.5) {
		led_set_all(1 << 1);
	}

	// NORTH EAST
	else if (heading >= 22.5 && heading < 67.5) {
		led_set_all(1 << 2);
	}

	// EAST
	else if (heading >= 67.5 && heading < 112.5) {
		led_set_all(1 << 3);
	}

	// SOUTH EAST
	else if (heading >= 112.5 && heading < 157.5) {
		led_set_all(1 << 4);
	}

	// SOUTH
	else if (heading >= 157.5 || heading < -157.5) {
		led_set_all(1 << 5);
	}

	// SOUTH WEST
	else if (heading >= -157.5 && heading < -112.5) {
		led_set_all(1 << 6);
	}

	// WEST
	else if (heading >= -112.5 && heading < -67.5) {
		led_set_all(1 << 7);
	}

	// NORTH WEST
	else if (heading >= -67.5 && heading < -22.5) {
		led_set_all(1 << 0);
	}


}

void writeHeadingToServo(float angle) {
	// angle is (-180, 180]

	// forces angle to be positive
	angle = fabs(angle);

	// now if |angle| >= 90
	// we want to invert it
	if (angle >= 90) {
		angle = 180 - angle;
	}

	servoWrite(angle);

}


uint32_t start = 0;

void receiveCallback(uint8_t* buffer, uint8_t size, uint8_t message_id) {

	// Winston
	// read and parse mag/button state data
	BoardMessage* message = (BoardMessage*) buffer;

	 char string[50];

	// rate limit sending string
	// send every 200ms
//	if (getNow() - start >= 200 * 1000) {
//		start = getNow();
//		snprintf(string, sizeof(string), "Heading: %.2f, Display: %d \r\n",
//				message->magSample.heading_deg,
//				message->displayState);
//		sendString(&USART1_PORT, string);
//	}

	if (message->displayState == 0) {

		servoWrite(0);
		// Jack
		// change LED state based on magnetometer
		writeHeadingToLEDs(message->magSample.heading_deg);

	} else {

		// turn off LEDs
		led_set_all(0);

		// Denny
		// change servo state based on magentometer
		writeHeadingToServo(message->magSample.heading_deg);

		// ensure we're not overloading the servo
		delayElapsed(30);
	}

}

void initializeBoardB() {
	serialInitialise(&UART4_PORT, BAUD_57600, receiveCallback);
//	serialInitialise(&USART1_PORT, BAUD_9600, 0x00);

	initElapsedTimer();


	servoInit();
	led_init();
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

	start = getNow();

	while (1) {
		// call receiveMsg here
		receiveMsg(&UART4_PORT);

		// Mock data
//		BoardMessage boardMsg;
//		boardMsg.displayState = 1;
//		boardMsg.magSample.valid = 1;
//		boardMsg.magSample.heading_deg = 180;
//
//		receiveCallback((uint8_t*) &boardMsg, sizeof(boardMsg), 0);

	}
}
