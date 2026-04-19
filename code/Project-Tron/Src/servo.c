#include "servo.h"
#include "pwm.h"
#include "io/gpio.h"



#define GPIO_CLASS_SERVO GPIOB
#define GPIO_SERVO 6

gpio_t gpioServo;

void onPWMRising(void* ptr) {
	gpio_write(&gpioServo, 1);
}

void onPWMFalling(void* ptr) {
	gpio_write(&gpioServo, 0);
}

void servoInit() {
	gpio_init(&gpioServo, GPIO_CLASS_SERVO, GPIO_SERVO, GPIO_MODE_OUTPUT);
}

void servoWrite(float angle) {
	// map [-90, 90] => [1, 2]
	uint32_t pwm = (uint32_t)(((angle + 90)/180 + 1) * 1000);

	pwm_init(20000, pwm, onPWMRising, onPWMFalling);

}

void testServo() {
	servoInit();

	double reverse = 0;
	float angle = -90;


	// 10 degrees per second
	double angular_rate = 120;

	// 30ms, 20 ms is the minimum since 20ms is the period of the PWM signal
	float period = 0.30;

	for (;;) {
		servoWrite(angle);


		if (angle >= 90) {
			reverse = 1;
		}
		if (angle <= -90) {
			reverse = 0;
		}

		if (reverse) {
			// delay is 10ms => period is 10ms
			// dtheta = w dt => angular_rate * periodms / 1000
			angle -=  angular_rate * period;
		} else {
			angle += angular_rate * period;
		}



		// 500ms
		uint32_t units_delay = (uint32_t) (period * 0xA2C2B);
		// 0xA2C2B -> "666667 clock cycles"
		// from testing (using a metronome) this is ~1 second
		for (uint32_t i = 0; i < units_delay; ++i) {}

	}
}
