#include <stdio.h>

#include "sensors/gyro.h"

#include "comm/uart.h"
#include "comm/spi.h"
#include "timer.h"

#define SENSITIVITY_MDPS 8.75

#define GYRO_WHO_AM_I_REG 0xF
#define GYRO_CTRL_REG1 0x20
#define GYRO_STATUS_REG 0x27
#define GYRO_OUT_X_L 0x28
#define GYRO_OUT_X_H 0x29
#define GYRO_OUT_Y_L 0x2A
#define GYRO_OUT_Y_H 0x2B
#define GYRO_OUT_Z_L 0x2C
#define GYRO_OUT_Z_H 0x2D


void initializeGyro() {
	initializeSPI();

	// turn on power, and Zen, Xen, Yen
	writeSPIRegister(GYRO_CTRL_REG1, 0b1111);
}

void readGyro(GyroRawData* data) {
	while (1) {
		uint8_t status = readSPIRegister(GYRO_STATUS_REG);
		if (status & (1 << 3)) {
			break;
		}
	}

	int16_t x = 0, y = 0, z = 0;

	x |= readSPIRegister(GYRO_OUT_X_L);
	x  |= readSPIRegister(GYRO_OUT_X_H) << 8;

	y |= readSPIRegister(GYRO_OUT_Y_L);
	y |= readSPIRegister(GYRO_OUT_Y_H) << 8;

	z |= readSPIRegister(GYRO_OUT_Z_L);
	z |= readSPIRegister(GYRO_OUT_Z_H) << 8;

	// convert values into deg/second
	data->x = ((float) x) * SENSITIVITY_MDPS * 0.001;
	data->y = ((float) y) * SENSITIVITY_MDPS * 0.001;
	data->z = ((float) z) * SENSITIVITY_MDPS * 0.001;

}





void testGyro() {
	serialInitialise(&USART1_PORT, BAUD_9600, 0x00);
	initElapsedTimer();

	initializeSPI();

	float thetaX = 0;
	float thetaY = 0;
	float thetaZ = 0;

	uint32_t start = getNow();

	while (1) {

		GyroRawData vel = {};

		readGyro(&vel);

		char string[50];

		// time elapsed in microseconds
		uint32_t time = getElapsed();

		thetaX = thetaX + (vel.x * time)/1000000;
		thetaY = thetaY + (vel.y * time)/1000000;
		thetaZ = thetaZ + (vel.z * time)/1000000;


		// rate limit sending string
		// send every 200ms
		if (getNow() - start >= 200 * 1000) {
			start = getNow();
			snprintf(string, sizeof(string), "%.2f, %.2f, %.2f\r\n", thetaX, thetaY, thetaZ);
			sendString(&USART1_PORT, string);
		}


		delayElapsed(1);

	}

}
