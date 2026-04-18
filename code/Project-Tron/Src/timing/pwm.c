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

static uint16_t period_us;
static uint16_t duty_cycle_us;

static void (*rising_edge_cb)(void *args) = NULL;
static void (*falling_edge_cb)(void *args)  = NULL;

void pwm_init(	uint16_t new_pwm_period_us, uint16_t new_duty_cycle_us,
				void (*new_rising_edge_cb)(void *args),
				void (*new_falling_edge_cb)(void *args))
{
	pwm_set_period_us(new_pwm_period_us);
	pwm_set_duty_cycle_us(new_duty_cycle_us);
	pwm_set_rising_edge_cb(new_rising_edge_cb);
	pwm_set_falling_edge_cb(new_falling_edge_cb);

	// start timer at duty_cycle period, PWM logic handles the rest
	timer_init(duty_cycle_us, pwm_callback);
}

void pwm_set_period_us(uint16_t new_pwm_period_us){
	period_us = new_pwm_period_us;
}

void pwm_set_duty_cycle_us(uint16_t new_pwm_duty_cycle_us){
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
        timer_set_period_us(duty_cycle_us);    	// wait for pulse width (e.g. 1500us)
        state = PWM_STATE_LOW;
    } else {
    	if (falling_edge_cb != NULL) falling_edge_cb(NULL);
        timer_set_period_us(period_us - duty_cycle_us); // wait for remainder of 20ms
        state = PWM_STATE_HIGH;
    }
}


gpio_t gpioPB6;

void onPWMRising(void* ptr) {
	gpio_write(&gpioPB6, 1);
}

void onPWMFalling(void* ptr) {
	gpio_write(&gpioPB6, 0);
}

void testPWM() {
	gpio_init(&gpioPB6, GPIOB, 6, GPIO_MODE_OUTPUT);

	double reverse = 0;
	int32_t angle = -90;

	// 10 degrees per second
	double angular_rate = 120;

	// 30ms, 20 ms is the minimum since 20ms is the period of the PWM signal
	float period = 0.30;

	for (;;) {

		// convert angle to pwm
		uint32_t pwm = (uint32_t)(((((float) angle) + 90)/180 + 1) * 1000);

		pwm_init(20000, pwm, onPWMRising, onPWMFalling);


		if (angle >= 90) {
			reverse = 1;
		}
		if (angle <= -90) {
			reverse = 0;
		}

		if (reverse) {
			// delay is 10ms => period is 10ms
			// dtheta = w dt => angular_rate * periodms / 1000
			angle -= angular_rate * period;
		} else {
			angle += angular_rate * period;;
		}



		// 500ms
		uint32_t units_delay = (uint32_t) (period * 0xA2C2B);
		// 0xA2C2B -> "666667 clock cycles"
		// from testing (using a metronome) this is ~1 second
		for (uint32_t i = 0; i < units_delay; ++i) {}
	}


}
