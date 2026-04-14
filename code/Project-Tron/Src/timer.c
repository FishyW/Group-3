#include "timer.h"
#include <stdlib.h>
#include "stm32f303xc.h"

static uint16_t period;
static uint16_t delay;

static void (*period_cb)(void *args) = NULL;
static void (*delay_cb)(void *args)  = NULL;

void timer_init(uint16_t user_period,
                void (*user_period_cb)(void *args))
{
    timer_set_period(user_period);
    timer_set_period_cb(user_period_cb);

    enable_timer();
}

void enable_timer(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->PSC   = 7;
    TIM2->ARR   = period * 1000;
    TIM2->DIER |= TIM_DIER_UIE;
    TIM2->CR1  |= TIM_CR1_CEN;

    NVIC_EnableIRQ(TIM2_IRQn);
}

// Setters
void timer_set_period(uint16_t new_period) {
    period = new_period;
    TIM2->ARR = period * 1000;
    TIM2->CNT = 0;
}

void timer_set_delay(uint16_t new_delay) {
    delay = new_delay;
    TIM2->CCR1 = delay * 1000;
    TIM2->CNT  = 0;
}

void timer_set_period_cb(void (*callback)(void *args)) {
    period_cb = callback;
}

void timer_set_delay_cb(void (*callback)(void *args)) {
    delay_cb = callback;
}

// Getters
uint16_t timer_get_period(void) {
    return period;
}

uint16_t timer_get_oneshot(void) {
    return delay;
}

// Advanced Functionality: Oneshot function.
void timer_oneshot_call(uint16_t user_delay,
                        void (*user_delay_cb)(void *args))
{
    timer_set_delay(user_delay);
    timer_set_delay_cb(user_delay_cb);

    TIM2->DIER |= TIM_DIER_CC1IE;
}

// Interrupt handler.
void TIM2_IRQHandler(void) {
    // Check Periodic (ARR)
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;

        if (period_cb != NULL) {
            period_cb(NULL);
        }
    }

    // Check Oneshot (CC1IF)
    if (TIM2->SR & TIM_SR_CC1IF) {
        TIM2->SR   &= ~TIM_SR_CC1IF;
        TIM2->DIER &= ~TIM_DIER_CC1IE;

        if (delay_cb != NULL) {
            delay_cb(NULL);
            delay_cb = NULL;
        }
    }
}
