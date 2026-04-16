#include "comm/i2c.h"
#include "comm/uart.h"
#include "timer.h"

#include "sensors/accel.h"

#include <stdio.h>

#define ACCEL_ADDRESS 0x19

#define WHO_AM_I_A 0xF
#define ACCEL_CTRL_REG1_A 0x20
#define ACCEL_CTRL_REG4_A 0x23
#define ACCEL_STATUS_REG_A 0x27
#define ACCEL_OUT_X_L_A 0x28
#define ACCEL_OUT_X_H_A 0x29
#define ACCEL_OUT_Y_L_A 0x2A
#define ACCEL_OUT_Y_H_A 0x2B
#define ACCEL_OUT_Z_L_A 0x2C
#define ACCEL_OUT_Z_H_A 0x2D

#define MG_PER_DIGIT 1

uint8_t readI2CRegister(uint8_t addr) {
	uint8_t value;
	i2cReadReg(ACCEL_ADDRESS, addr, &value);
	return value;
}

void writeI2CRegister(uint8_t addr, uint8_t data) {
	i2cWriteReg(ACCEL_ADDRESS, addr, data);
}

void initializeAccel() {
	i2c1Init();
	// ODR = 0101 -> 100Hz data rate
	// Xen, Yen, Zen = 1 => enable all axis
	writeI2CRegister(ACCEL_CTRL_REG1_A, 0b01010111);

	writeI2CRegister(ACCEL_CTRL_REG4_A, 0b10001000);
}

void readAccel(AccelRawData* data) {
	while (1) {
		uint8_t status = readI2CRegister(ACCEL_STATUS_REG_A);
		if ((status & 1 << 3)) {
			break;
		}
	}

	 uint8_t raw[6];


	if (!i2cReadRegs(ACCEL_ADDRESS, ACCEL_OUT_X_L_A, raw, 6U, true))
		     return ;

	int16_t x = 0, y = 0, z = 0;


	// shift right by 4 since it's 12-bit left aligned register
	x |= (int16_t)((raw[1] << 8) | raw[0]) >> 4;

	y |=  (int16_t)((raw[3] << 8) | raw[2]) >> 4;

	z |=  (int16_t)((raw[5] << 8) | raw[4]) >> 4;



	// convert values into g (e.g. 3 g = 3 * 9.8 m/s^2)
	data->x = ((float) x) * MG_PER_DIGIT * 0.001;
	data->y = ((float) y) * MG_PER_DIGIT * 0.001;
	data->z = ((float) z) * MG_PER_DIGIT * 0.001;

}



void testAccel() {

	serialInitialise(&USART1_PORT, BAUD_9600, 0x00);
	initElapsedTimer();

	initializeAccel();

	uint32_t start = getNow();
	while (1) {
		AccelRawData data;
		readAccel(&data);

		// rate limit sending string
		// send every 200ms

		if (getNow() - start >= 200 * 1000) {
			start = getNow();
			char string[50];
			snprintf(string, sizeof(string), "%.2f, %.2f, %.2f\r\n", data.x, data.y, data.z);
			sendString(&USART1_PORT, string);
		}

		delayElapsed(10);
	}


}
