#ifndef GYRO_H
#define GYRO_H

#include <stdint.h>

// x, y, z in degree/second
typedef struct {
	float x;
	float y;
	float z;
} GyroRawData;

void initializeGyro();

void testGyro();

#endif
