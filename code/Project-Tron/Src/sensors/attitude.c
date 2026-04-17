#include "sensors/attitude.h"
#include "sensors/accel.h"
#include "sensors/gyro.h"
#include "sensors/magnetometer.h"

#include "comm/uart.h"
#include "comm/spi.h"
#include "comm/i2c.h"

#include "timer.h"

#include <math.h>
#include <stdio.h>

void testAttitude() {
	serialInitialise(&USART1_PORT, BAUD_9600, 0x00);
	initElapsedTimer();

	initializeSPI();
	initializeAccel();
	magInit();

	uint32_t start = getNow();
	while (1) {
		AccelRawData accel;
		readAccel(&accel);

		MagSample mag;
		magReadSample(&mag);


		// https://ahrs.readthedocs.io/en/latest/filters/complementary.html
		// first compute thetax and thetay
		float thetax = -atan2(accel.y, accel.z);
		float thetay = atan2(-accel.x, hypot(accel.y, accel.z));


		// float thetaz = (M_PI / 180.0f) * mag.heading_deg - M_PI;


		// rate limit sending string
		// send every 200ms

		if (getNow() - start >= 200 * 1000) {
			start = getNow();
			char string[50];
			snprintf(string, sizeof(string), "%d, %d, %d, %.2f\r\n", mag.raw_x, mag.raw_y, mag.raw_z, mag.heading_deg);
			sendString(&USART1_PORT, string);
		}

		// query every 100 Hz
		delayElapsed(10);
	}



}
