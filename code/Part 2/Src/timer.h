#ifndef TIMER_H
#define TIMER_H

uint8_t event_period;	// in ms

// enable_timer: enables the STM32F3Discovery timers.
void enable_timer(void);

// periodic_event: calls callback_function every event_period ms.
void periodic_event(function callback_function);

// oneshot_event: calls callback_function after delay ms.
void oneshot_event(uint8_t delay, function callback_function);

// get_timer_period: returns the event_period in this module.
uint16_t get_timer_period(void);

//set_timer_period: writes new_period argument as event_period.
void set_timer_period(uint16_t new_period);


#endif
