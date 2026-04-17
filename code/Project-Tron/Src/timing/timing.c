/*
 * CONTAINS HIGH LEVEL UTILITIES THAT USES timer.c
 */
#include "timer.h"
#include "io/led.h"

#define ELAPSED_PERIOD_MS 500

// elapsed rounded to the nearest 500ms
// in microseconds
uint32_t elapsedRounded = 0;

uint32_t before = 0;


void periodCallback(void* value) {
	elapsedRounded += ELAPSED_PERIOD_MS * 1000;
}

void initElapsedTimer() {
	timer_init(ELAPSED_PERIOD_MS, periodCallback);
}


uint32_t getNow() {
	return timer_get_elapsed() + elapsedRounded;
}

uint32_t getElapsed() {
	uint32_t now = getNow();
	uint32_t elapsed = now - before;
	before = now;
	return elapsed;
}


void delayElapsed(uint16_t time) {
	uint32_t start = getNow();
	while (getNow() - start < time * 1000) {}
}

uint8_t delayFlag = 0;

void delayCallback(void* value) {
	delayFlag = 1;
}


void delay(uint16_t time) {
	delayFlag = 0;
	timer_oneshot_call(time, delayCallback);
	while (!delayFlag) {}
}

/**
 * Test Functions
 */

void testDelay() {
	led_init();
	initElapsedTimer();

	uint16_t counter = 0;

for (;;) {
		delay(1000);
		++counter;
		led_set_all(counter);

	}
}

