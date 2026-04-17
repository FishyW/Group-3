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

#define BUFFER_SIZE 10

// if velocity is >= 1m/s => increment using gyro
#define DELTAX_SQUARED_THRESHOLD 0.1

typedef struct {
	float data[BUFFER_SIZE];
	int index;
} CircularBuffer;

void pushBuffer(CircularBuffer* buffer, float value) {
	buffer->data[buffer->index] = value;
	buffer->index = (buffer->index + 1) % BUFFER_SIZE;
}

float maxElement(CircularBuffer* buffer) {
	// use -1 since all values are positive
	float maximum = -1;
	for (int i = 0; i < BUFFER_SIZE; ++i) {
		maximum = fmax(maximum, buffer->data[i]);
	}
	return maximum;
}

typedef struct {
	float r;
	float p;
	float y;
} Attitude;

void attitudeFromAccelAndMag(Attitude* attitude) {

	AccelRawData accel;
	readAccel(&accel);

	MagSample mag;
	magReadSample(&mag);

	// https://ahrs.readthedocs.io/en/latest/filters/complementary.html
	// first compute thetax and thetay
	float thetax = -atan2(accel.y, accel.z);
	float thetay = atan2(-accel.x, hypot(accel.y, accel.z));

	// from accel
	// thetax (roll) = x axis (forward/backward)
	// thetay (pitch) = y axis (left/right)
	// thetaz

	// float thetaz = (M_PI / 180.0f) * mag.heading_deg - M_PI;

	// tilt compensated thetaz
	float bx = mag.raw_y * cos(thetax) + sin(thetax)
			* (mag.raw_x * sin(thetay) + mag.raw_z * cos(thetay));

	float by = mag.raw_x * cos(thetay) - mag.raw_z * sin(thetay);

	float thetaz = atan2(-by, bx);

	// from the sensor POV pitch and roll are swapped
	attitude->p = thetax * 180/M_PI;
	attitude->r = thetay * 180/M_PI;
	attitude->y = thetaz * 180/M_PI;
}

void deltaAttitudeFromGyro(Attitude* attitude) {
	GyroRawData vel = {};
	readGyro(&vel);

	// time elapsed in microseconds
	uint32_t time = getElapsed();

	// from the sensor POV pitch and roll are swapped
	attitude->p = (vel.x * time)/1000000;
	attitude->r = (vel.y * time)/1000000;
	attitude->y = (vel.z * time)/1000000;

}

void testAttitude() {
	serialInitialise(&USART1_PORT, BAUD_9600, 0x00);
	initElapsedTimer();

	initializeGyro();
	initializeAccel();
	magInit();

	uint32_t start = getNow();

	CircularBuffer dXBuffer = {};
	Attitude previousAttitude = {0,0,0};

	while (1) {

		// main idea with algorithm
		// observe that when stable mag + accel data is good
		// but when not stable gyro data is better

		// if the max |delta X| in a rolling window is high
		// use gyro to increment

		Attitude attitudeMagAccel;
		Attitude deltaAttitudeGyro;

		attitudeFromAccelAndMag(&attitudeMagAccel);

		deltaAttitudeFromGyro(&deltaAttitudeGyro);

		Attitude finalAttitude;

		// check stability
		float dX2 = pow(deltaAttitudeGyro.p,2) + pow(deltaAttitudeGyro.r, 2) + pow(deltaAttitudeGyro.y,2);
		pushBuffer(&dXBuffer, dX2);

		float stability = maxElement(&dXBuffer);

		if (stability < DELTAX_SQUARED_THRESHOLD) {
			finalAttitude.r = deltaAttitudeGyro.r + previousAttitude.r;
			finalAttitude.p = deltaAttitudeGyro.p + previousAttitude.p;
			finalAttitude.y = deltaAttitudeGyro.y + previousAttitude.y;
		} else {
			finalAttitude = attitudeMagAccel;
		}

		previousAttitude = finalAttitude;

		// rate limit sending string
		// send every 200ms

		if (getNow() - start >= 200 * 1000) {
			start = getNow();
			char string[50];
			 snprintf(string, sizeof(string), "Roll: %.2f, Pitch: %.2f, Yaw: %.2f\r\n",
					finalAttitude.r, finalAttitude.p, finalAttitude.y);
			sendString(&USART1_PORT, string);
		}

		// query every 100 Hz
		delayElapsed(10);
	}



}
