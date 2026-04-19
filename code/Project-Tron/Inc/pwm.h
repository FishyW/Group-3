#ifndef PWM_H
#define PWM_H

#include "stm32f303xc.h"
#include <stddef.h>

// ─── public functions ─────────────────────────────────────

void pwm_init(uint32_t new_pwm_period,
			uint32_t new_duty_cycle,
              void (*new_rising_edge_cb)(void *args),
              void (*new_falling_edge_cb)(void *args));

// setters
void pwm_set_period_us(uint32_t new_pwm_period_us);
void pwm_set_duty_cycle_us(uint32_t new_pwm_duty_cycle_us);
void pwm_set_rising_edge_cb(void (*new_rising_edge_cb)(void *args));
void pwm_set_falling_edge_cb(void (*new_falling_edge_cb)(void *args));

// callback — registered with timer module, not called directly
void pwm_callback(void *args);

void testPWM();

#endif
