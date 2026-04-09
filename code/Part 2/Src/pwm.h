#ifndef PWM_H
#define PWM_H

#include 'timer.h'

uint8_t duty_cycle

// Part C seems simple enough?

// The idea is: Set GPIO pins at a specific interval.





void generate_pwm_signal_0(){
	GPIO(HIGH);
	oneshot_function(duty_cycle, GPIO(LOW));
}

void generate_pwm_signal(){
	periodic_function()
}

periodic_function(generate_pwm_signal){
	generate_pwm_signal
}
