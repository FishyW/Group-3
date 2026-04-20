#include "timer.h"
#include <stdlib.h>
#include "stm32f303xc.h"

// Struct of a Timer that encapsulates all necessary info.
typedef struct _TimerInfo {
	TIM_TypeDef* timer;
	volatile uint32_t RCCAPB1EnMask;
	volatile uint32_t IRQn;
	uint32_t period_us;
	uint8_t one_shot_mode;
	void (*timer_cb)(void *args);
} TimerInfo;

// Using above struct to define some timers:
TimerInfo TIM2_TIMER = {
	TIM2,
	RCC_APB1ENR_TIM2EN,
	TIM2_IRQn,
	0,
	0,
	0x00
};

// TIM3_TIMER, note all timers besides TIM2 use 16 bit registers
TimerInfo TIM3_TIMER = {
	TIM3,
	RCC_APB1ENR_TIM3EN,
	TIM3_IRQn,
	0,
	0,
	0x00
};

// Interrupt handlers:
void TIM2_IRQHandler(void) {
   timer_irq(&TIM2_TIMER);
}

void TIM3_IRQHandler(void) {
   timer_irq(&TIM3_TIMER);
}


void setup_timer(TimerInfo* info) {
    RCC->APB1ENR |= info->RCCAPB1EnMask; // Enable the corresponding bit defined in the struct.
    info->timer->PSC  = 7;	// 8Mhz
    NVIC_EnableIRQ(info->IRQn); // Enable the relevant Interrupt handler.
}


void timer_init(TimerInfo* info,
				uint32_t user_period_us,
				void (*user_period_cb)(void *args))
{
	setup_timer(info);
	timer_set_period(info, user_period_us);
	timer_set_timer_cb(info, user_period_cb);
}

// Utilities
// timer_start: Enables the counter and allowing interrupts from timer.
void timer_start(TimerInfo *info){
	info->timer->CR1  |= TIM_CR1_CEN;
	info->timer->DIER |= TIM_DIER_UIE;
}

// timer_start: Disables counter counting and interrupts.
void timer_stop(TimerInfo *info){
	info->timer->CR1  &= ~TIM_CR1_CEN;
	info->timer->DIER &= ~TIM_DIER_UIE;
}

// timer_reset_coutner: Sets CNT to 0 and clears the interrupt flag.
void timer_reset_counter(TimerInfo *info){
	info->timer->CNT = 0;
	info->timer->EGR = 1;
	__DSB();
	info->timer->SR &= ~TIM_SR_UIF;
}

// Setters
// timer_set_period_us: set the period in microseconds
void timer_set_period_us(TimerInfo* info, uint32_t new_period_us) {
	// Stop the timer, set the time, reset, then start it again.
	timer_stop(info);
	// reset the period
	info->period_us = new_period_us;
	info->timer->ARR = info->period_us;
	timer_reset_counter(info);
	timer_start(info);
}

//timer_set_period: Outdated function that converts from micro to milliseconds
void timer_set_period(TimerInfo* info, uint32_t new_period){
	timer_set_period_us(info, new_period*1000);
}

//timer_set_timer_cb: Sets the timer's cb that triggers after a time. The mode will determine whether counter restarts, but the function is the same.
void timer_set_timer_cb(TimerInfo* info, void (*callback)(void *args)) {
    info->timer_cb = callback;
}

// timer_set_mode: Stores the mode in the struct, and changes the OPM (One-pulse-mode) bit.
void timer_set_mode(TimerInfo *info, uint8_t mode){
	timer_stop(info);
	timer_reset_counter(info);
	info->one_shot_mode = mode;
	if (mode == 0)
		info->timer->CR1 &= ~TIM_CR1_OPM; // turn off if one_shot_mode is off
	else
		info->timer->CR1 |= TIM_CR1_OPM;
	timer_start(info);
}

// Getters
// timer_get_period_us: Returns the period we desire in us.
uint32_t timer_get_period_us(TimerInfo* info) {
    return info->period_us;
}

// timer_get_elapsed: Returns the current counter
uint32_t timer_get_elapsed(TimerInfo* info) {
	return info->timer->CNT;
}

// Advanced Functionality: Oneshot function.
void timer_oneshot_call(TimerInfo* info, uint32_t user_delay,
                        void (*user_delay_cb)(void *args))
{
    timer_set_period(info, user_delay);
    timer_set_timer_cb(info, user_delay_cb);
    timer_set_mode(info, 1);
    timer_start(info);
}

void timer_irq(TimerInfo* info) {
	 // Check Periodic (ARR)
	if (info->timer->SR & TIM_SR_UIF) {
		info->timer->SR &= ~TIM_SR_UIF;	// Must manually switch off

		if (info->timer_cb != NULL) {
			info->timer_cb(NULL);
		}
	}
}

// Test functions for demo:

// Demonstrate that a callback function can be triggered at regular intervals:

void part_a_LED_companion(void *args){
	GPIOE->ODR ^= (1 << 8);  // toggle PE8 (adjust pin as needed)
}

void timer_demo_part_a(){
	RCC->AHBENR |= RCC_AHBENR_GPIOEEN;  // enable GPIOE clock
	GPIOE->MODER |=  (1 << 16);         // set PE8 as output
	timer_init(&TIM2_TIMER,
				1000,
				part_a_LED_companion
	);
}

// Demonstrate a new period can be set
void timer_demo_part_b(){
	timer_demo_part_a(); // Get a blink going at 1000 milliseconds
	timer_set_period(&TIM2_TIMER, 100);
}

void part_d_LED_companion(void *args){
	GPIOE->ODR ^= (1 << 8);
}

void timer_demo_part_d(){
	timer_demo_part_a();
	timer_oneshot_call(&TIM2_TIMER, 500, part_d_LED_companion);
}
