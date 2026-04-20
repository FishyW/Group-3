#ifndef TIMER_H
#define TIMER_H

#include "stm32f303xc.h"
#include <stddef.h>

struct _TimerInfo;
typedef struct _TimerInfo TimerInfo;

extern TimerInfo TIM2_TIMER;
extern TimerInfo TIM3_TIMER;

// timer_init: Sets callback functions for periodic and oneshot, and time delay for each.
// Also enables the timer and starts it.
void timer_init(
	TimerInfo* info,
	uint32_t user_period,
    void (*user_period_cb)(void *args));

void enable_timer(void);

// setters
void timer_set_period_us(TimerInfo* info, uint32_t new_period_us);
void timer_set_period(TimerInfo* info, uint32_t new_period);

void timer_set_period_cb(TimerInfo* info, void (*callback)());

// getters
uint32_t timer_get_period_us(TimerInfo* info);


// get the counter's value (time elapsed in microseconds)
uint32_t timer_get_elapsed(TimerInfo* info);

// Advanced Functionality
void timer_oneshot_call(TimerInfo* info, uint32_t user_delay,
                        void (*user_delay_cb)());

void timer_irq(TimerInfo* info);

void timer_set_timer_cb(TimerInfo* info, void (*callback)(void *args));

void timer_demo_part_a();

void timer_demo_part_b();

void initElapsedTimer();

void timer_demo_part_d();

// get the current time relative to when initElapsedTimer is called
// the unit is in microseconds
uint32_t getNow();

// get the time that has passed between getElapsed function calls
// the unit is in microseconds
uint32_t getElapsed();

/*
 * The delay functions below takes in time in milliseconds
 * And waits until the specified time has passed
 */

// delay for time amount of milliseconds
// variant that uses elapsed time
// before calling this initElapsedTimer must already be called!
void delayElapsed(uint16_t time);

// variant that doesn't use elapsed time
// it uses one shot
void delay(uint16_t time);

/**
 * Test Functions
 */

void testDelay();

#endif
