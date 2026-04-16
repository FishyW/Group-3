#ifndef TIMER_H
#define TIMER_H

#include "stm32f303xc.h"
#include <stddef.h>

// timer_init: Sets callback functions for periodic and oneshot, and time delay for each.
// Also enables the timer and starts it.
void timer_init(uint16_t user_period,
                void (*user_period_cb)(void *args));

void enable_timer(void);

// setters
void timer_set_period(uint16_t new_period);
void timer_set_delay(uint16_t new_delay);
void timer_set_period_cb(void (*callback)(void *args));
void timer_set_delay_cb(void (*callback)(void *args));

// getters
uint16_t timer_get_period(void);
uint16_t timer_get_delay(void);

// Advanced Functionality
void timer_oneshot_call(uint16_t user_delay,
                        void (*user_delay_cb)(void *args));

#endif
