#include "pwm.h"
#include <stdlib.h>
#include "stm32f303xc.h"
#include "timer.h"


// THIS FILE IS LESS EFFICIENT TO ABIDE BY ASSIGNMENT

typedef enum {
    PWM_STATE_HIGH,
    PWM_STATE_LOW
} pwm_state_t;

static pwm_state_t state = PWM_STATE_HIGH;

static uint16_t period;
static uint16_t duty_cycle;

static void (*rising_edge_cb)(void *args) = NULL;
static void (*falling_edge_cb)(void *args)  = NULL;

void pwm_init(	uint16_t new_pwm_period, uint16_t new_duty_cycle,
				void (*new_rising_edge_cb)(void *args),
				void (*new_falling_edge_cb)(void *args))
{
	pwm_set_period(new_pwm_period);
	pwm_set_duty_cycle(new_duty_cycle);
	pwm_set_rising_edge_cb(new_rising_edge_cb);
	pwm_set_falling_edge_cb(new_falling_edge_cb);

	// start timer at duty_cycle period, PWM logic handles the rest
	timer_init(duty_cycle, pwm_callback);
}

void pwm_set_period(uint16_t new_pwm_period){
	period = new_pwm_period;
}

void pwm_set_duty_cycle(uint16_t new_pwm_duty_cycle){
	duty_cycle = new_pwm_duty_cycle;
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
        timer_set_period(duty_cycle);    	// wait for pulse width (e.g. 1500us)
        state = PWM_STATE_LOW;
    } else {
    	if (falling_edge_cb != NULL) falling_edge_cb(NULL);
        timer_set_period(period - duty_cycle); // wait for remainder of 20ms
        state = PWM_STATE_HIGH;
    }
}


