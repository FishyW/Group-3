#include "timer.h"
#include <stdlib.h>
#include "stm32f303xc.h"


static uint32_t period_us;
static uint8_t one_shot_mode;

static void (*period_cb)(void *args) = NULL;

void timer_init(uint32_t user_period,
                void (*user_period_cb)(void *args))
{
    timer_set_period(user_period);
    timer_set_period_cb(user_period_cb);

    enable_timer();
}

void enable_timer(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->PSC   = 7;
    TIM2->ARR   = period_us;
    TIM2->DIER |= TIM_DIER_UIE;
    TIM2->CR1  |= TIM_CR1_CEN;

    NVIC_EnableIRQ(TIM2_IRQn);
}

// set the period in microseconds
void timer_set_period_us(uint32_t new_period_us) {
	// disable the clock first
		TIM2->CR1  &= ~TIM_CR1_CEN;
		TIM2->DIER &= ~TIM_DIER_UIE;


		// reset the period
	    period_us = new_period_us;
	    TIM2->ARR = period_us;
	    TIM2->CNT = 0;

	    // set UG bit to 1, this forces the registers to update
	    TIM2->EGR = 1;


	    // wait for EGR to update the UIF bit
	    // we need DSB since it may take a while for EGR to update UIF
	    __DSB();

		// clear UIF bit
		TIM2->SR &= ~TIM_SR_UIF;


	    // reenable the clock
	    TIM2->CR1  |= TIM_CR1_CEN;
	    TIM2->DIER |= TIM_DIER_UIE;
}

// Setters

// sets the period in milliseconds
void timer_set_period(uint32_t new_period) {
	timer_set_period_us(new_period * 1000);
}


void timer_set_period_cb(void (*callback)(void *args)) {
    period_cb = callback;
}


// Getters
uint32_t timer_get_period_us(void) {
    return period_us;
}

uint32_t timer_get_period(void) {
    return period_us * 1000;
}


uint32_t timer_get_elapsed(void) {
	return TIM2->CNT;
}

// Advanced Functionality: Oneshot function.
void timer_oneshot_call(uint32_t user_delay,
                        void (*user_delay_cb)(void *args))
{
    timer_set_period(user_delay);
    timer_set_period_cb(user_delay_cb);
    one_shot_mode = 1;
}

// Interrupt handler.
void TIM2_IRQHandler(void) {
    // Check Periodic (ARR)
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;

        if (one_shot_mode) {
        	one_shot_mode = 0;

        	// reset clock
        	timer_set_period_us(period_us);

        	// disable the clock
        	TIM2->CR1  &= ~TIM_CR1_CEN;

        }

        if (period_cb != NULL) {
            period_cb(NULL);
        }
    }
}




