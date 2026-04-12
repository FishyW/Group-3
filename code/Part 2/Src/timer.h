#ifndef TIMER_H
#define TIMER_H

#include "stm32f303xc.h"
#include <stddef.h>

// timer_init: Sets callback functions for periodic and oneshot, and time delay for each.
// Also enables the timer and starts it.
void timer_init(uint16_t desired_period,
                void (*callback_periodic)(void *args),
                uint16_t desired_oneshot,
                void (*cb_oneshot)(void *args));

void enable_timer(void);

// setters
void timer_set_period(uint16_t new_period);
void timer_set_oneshot(uint16_t new_oneshot);
void timer_set_periodic_callback(void (*callback)(void *args));
void timer_set_oneshot_callback(void (*callback)(void *args));

// getters
uint16_t timer_get_period(void);
uint16_t timer_get_oneshot(void);

#endif
