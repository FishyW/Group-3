#include "pwm.h"
#include <stdlib.h>
#include "stm32f303xc.h"
#include "timer.h"


#include "io/gpio.h"

// THIS FILE IS LESS EFFICIENT TO ABIDE BY ASSIGNMENT

typedef enum {
    PWM_STATE_HIGH,
    PWM_STATE_LOW
} pwm_state_t;

static pwm_state_t state = PWM_STATE_HIGH;

static uint32_t period_us;
static uint32_t duty_cycle_us;

static void (*rising_edge_cb)(void *args) = NULL;
static void (*falling_edge_cb)(void *args)  = NULL;

void pwm_init(	uint32_t new_pwm_period_us, uint32_t new_duty_cycle_us,
				void (*new_rising_edge_cb)(void *args),
				void (*new_falling_edge_cb)(void *args))
{
	pwm_set_period_us(new_pwm_period_us);
	pwm_set_duty_cycle_us(new_duty_cycle_us);
	pwm_set_rising_edge_cb(new_rising_edge_cb);
	pwm_set_falling_edge_cb(new_falling_edge_cb);

	// start timer at duty_cycle period, PWM logic handles the rest
	// use TIM3 timer, note that this is fine since 20,000 (20ms) < 65,536 (65ms)
	timer_init(&TIM3_TIMER, duty_cycle_us/1000, pwm_callback);
}

void pwm_set_period_us(uint32_t new_pwm_period_us){
	period_us = new_pwm_period_us;
}

void pwm_set_duty_cycle_us(uint32_t new_pwm_duty_cycle_us){
	duty_cycle_us = new_pwm_duty_cycle_us;
}

void pwm_set_rising_edge_cb(void (*new_rising_edge_cb)(void *args)){
	rising_edge_cb = new_rising_edge_cb;
}

void pwm_set_falling_edge_cb(void (*new_falling_edge_cb)(void *args)){
	falling_edge_cb = new_falling_edge_cb;
}


void pwm_callback(void *args){
    if (state == PWM_STATE_HIGH) {
    	if (rising_edge_cb != NULL) rising_edge_cb(NULL);
        timer_set_period_us(&TIM3_TIMER, duty_cycle_us);    	// wait for pulse width (e.g. 1500us)
        state = PWM_STATE_LOW;
    } else {
    	if (falling_edge_cb != NULL) falling_edge_cb(NULL);
        timer_set_period_us(&TIM3_TIMER, period_us - duty_cycle_us); // wait for remainder of 20ms
        state = PWM_STATE_HIGH;
    }
}



