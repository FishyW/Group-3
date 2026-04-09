#ifndef PWM_H
#define PWM_H

#include 'timer.h'

uint8_t duty_cycle;

// generate_pulse: writes high to given GPIO pin, then after duty_cycle ms, writes low.
void generate_pulse(function GPIO_write);

// generate_pwm_signal: loops generate_pulse using periodic event.
void generate_pwm_signal();
