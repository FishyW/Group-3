#ifndef INTEGRATION_H
#define INTEGRATION_H

#include "integration.h"
#include "sensors/magnetometer.h"

#define DELAY_TIME_MILLISECONDS 100

typedef struct {
	MagSample magSample;
	uint8_t displayState;
} BoardMessage;

void runBoardA();

void runBoardB();


#endif
