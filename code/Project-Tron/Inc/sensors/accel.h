#ifndef ACCEL_H
#define ACCEL_H

#include <stdint.h>


// x, y, z in degree/second
typedef struct {
	float x;
	float y;
	float z;
} AccelRawData;

void initializeAccel();

void readAccel(AccelRawData* data);

void testAccel();

#endif
