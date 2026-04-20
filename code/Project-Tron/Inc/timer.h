#ifndef TIMER_H
#define TIMER_H

#include "stm32f303xc.h"
#include <stddef.h>

struct _TimerInfo;
typedef struct _TimerInfo TimerInfo;

// Timer Instances
extern TimerInfo TIM2_TIMER;
extern TimerInfo TIM3_TIMER;

// timer_init: Takes the instances and adds user period and callback.
void timer_init(TimerInfo* info, uint32_t user_period, void (*user_period_cb)(void *args));

// Utility Functions:
// timer_start: Enables corresponding timer and interrupts.
void timer_start(TimerInfo* info);
// timer_stop: Disables corresponding timer and interrupts.
void timer_stop(TimerInfo* info);
// timer_reset_counter: Clears interrupt flag and sets counter to 0.
void timer_reset_counter(TimerInfo* info);

// Setters:
void timer_set_period_us(TimerInfo* info, uint32_t new_period_us); // Setting in microseconds.
void timer_set_period(TimerInfo* info, uint32_t new_period);
void timer_set_timer_cb(TimerInfo* info, void (*callback)(void *args));

// Getters
uint32_t timer_get_period_us(TimerInfo* info);
uint32_t timer_get_elapsed(TimerInfo* info);

// Interrupt handler.
void timer_irq(TimerInfo* info);

// Advanced oneshot call
void timer_oneshot_call(TimerInfo* info, uint32_t user_delay, void (*user_delay_cb)(void *args));

// High level utilities in timing.h
void initElapsedTimer();
uint32_t getNow();      /* microseconds since initElapsedTimer()        */
uint32_t getElapsed();  /* microseconds since last getElapsed() call    */
void delayElapsed(uint16_t time_ms);
void delay(uint16_t time_ms);

// Demo:
void timer_demo_part_a();
void timer_demo_part_b();
void timer_demo_part_d();
void testDelay();

#endif
