#include "timer.h"
#include <stdlib.h>
#include "stm32f303xc.h"

typedef struct _TimerInfo {
	TIM_TypeDef* timer;
	volatile uint32_t RCCAPB1EnMask;
	volatile uint32_t IRQn;
	uint32_t period_us;
	uint8_t one_shot_mode;
	void (*period_cb)(void);
} TimerInfo;

TimerInfo TIM2_TIMER = {
	TIM2,
	RCC_APB1ENR_TIM2EN,
	TIM2_IRQn,
	0,
	0,
	0x00
};

// Interrupt handler.
void TIM2_IRQHandler(void) {
   timer_irq(&TIM2_TIMER);
}

// TIM3_TIMER, note all timers besides TIM2 use 16 bit registers
TimerInfo TIM3_TIMER = {
	TIM3,
	RCC_APB1ENR_TIM3EN,
	TIM3_IRQn,
	0,
	0,
	0x00
};

void TIM3_IRQHandler(void) {
   timer_irq(&TIM3_TIMER);
}


void setup_timer(TimerInfo* info) {
    RCC->APB1ENR |= info->RCCAPB1EnMask;

    info->timer->PSC  = 7;

    NVIC_EnableIRQ(info->IRQn);
}


void timer_init(
	TimerInfo* info,
	uint32_t user_period,
    void (*user_period_cb)(void *args))
{
	setup_timer(info);
	timer_set_period_cb(info, user_period_cb);
    timer_set_period(info, user_period);
}


// set the period in microseconds
void timer_set_period_us(TimerInfo* info, uint32_t new_period_us) {
	// disable the clock first
		info->timer->CR1  &= ~TIM_CR1_CEN;
		info->timer->DIER &= ~TIM_DIER_UIE;


		// reset the period
	    info->period_us = new_period_us;
	    info->timer->ARR = info->period_us;
	    info->timer->CNT = 0;

	    // set UG bit to 1, this forces the registers to update
	    info->timer->EGR = 1;


	    // wait for EGR to update the UIF bit
	    // we need DSB since it may take a while for EGR to update UIF
	    __DSB();

		// clear UIF bit
	    info->timer->SR &= ~TIM_SR_UIF;


	    // reenable the clock
	    info->timer->CR1  |= TIM_CR1_CEN;
	    info->timer->DIER |= TIM_DIER_UIE;
}

// Setters

// sets the period in milliseconds
void timer_set_period(TimerInfo* info, uint32_t new_period) {
	timer_set_period_us(info, new_period * 1000);
}


void timer_set_period_cb(TimerInfo* info, void (*callback)(void)) {
    info->period_cb = callback;
}


// Getters
uint32_t timer_get_period_us(TimerInfo* info) {
    return info->period_us;
}

uint32_t timer_get_period(TimerInfo* info) {
    return info->period_us * 1000;
}


uint32_t timer_get_elapsed(TimerInfo* info) {
	return info->timer->CNT;
}

// Advanced Functionality: Oneshot function.
void timer_oneshot_call(TimerInfo* info, uint32_t user_delay,
                        void (*user_delay_cb)())
{
    timer_set_period(info, user_delay);
    timer_set_period_cb(info, user_delay_cb);
    info->one_shot_mode = 1;
}

void timer_irq(TimerInfo* info) {
	 // Check Periodic (ARR)
	if (info->timer->SR & TIM_SR_UIF) {
		info->timer->SR &= ~TIM_SR_UIF;

		if (info->one_shot_mode) {
			info->one_shot_mode = 0;

			// reset clock
			timer_set_period_us(info, info->period_us);

			// disable the clock
			info->timer->CR1  &= ~TIM_CR1_CEN;

		}

		if (info->period_cb != NULL) {
			info->period_cb();
		}
	}
}





