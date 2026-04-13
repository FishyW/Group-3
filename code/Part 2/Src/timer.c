#include "timer.h"
#include <stdlib.h>
#include "stm32f303xc.h"

static uint16_t event_period;
static uint16_t event_oneshot;

static void (*periodic_callback)(void *args) = NULL;
static void (*oneshot_callback)(void *args)  = NULL;

void timer_init(uint16_t desired_period,
                void (*callback_periodic)(void *args),
                uint16_t desired_oneshot,
                void (*cb_oneshot)(void *args))
{
    timer_set_period(desired_period);
    timer_set_periodic_callback(callback_periodic);
    timer_set_oneshot(desired_oneshot);
    timer_set_oneshot_callback(cb_oneshot);

    enable_timer();
}

void enable_timer(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->PSC   = 7;                      // 1MHz tick rate
    TIM2->ARR   = event_period * 1000;    // ms to ticks
    TIM2->CCR1  = event_oneshot * 1000;   // oneshot threshold
    TIM2->DIER |= TIM_DIER_UIE;           // periodic interrupt
    TIM2->DIER |= TIM_DIER_CC1IE;         // oneshot interrupt
    TIM2->CR1  |= TIM_CR1_CEN;

    NVIC_EnableIRQ(TIM2_IRQn);
}

// Setters
void timer_set_period(uint16_t new_period) {
    event_period = new_period;
    TIM2->ARR    = event_period * 1000;
    TIM2->CNT = 0;        // or update event
}

void timer_set_oneshot(uint16_t new_oneshot) {
    event_oneshot = new_oneshot;
    TIM2->CCR1    = event_oneshot * 1000;
    TIM2->CNT = 0;        // or update event
}

void timer_set_periodic_callback(void (*callback)(void *args)) {
    periodic_callback = callback;
}

void timer_set_oneshot_callback(void (*callback)(void *args)) {
    oneshot_callback = callback;
}

// Getters
uint16_t timer_get_period(void) {
    return event_period;
}

uint16_t timer_get_oneshot(void) {
    return event_oneshot;
}

// Interrupt handler.
void TIM2_IRQHandler(void) {
	// Check Periodic (ARR)
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;	// Clear flag

        if (periodic_callback != NULL) {
            periodic_callback(NULL);
        }
    }

    // Check Oneshot (CC1IF)
    if (TIM2->SR & TIM_SR_CC1IF) {
        TIM2->SR  &= ~TIM_SR_CC1IF;		// Clear flag
        TIM2->DIER &= ~TIM_DIER_CC1IE;  // disable CC1 for oneshot. Otherwise it will repeat every clock reset from ARR.

        if (oneshot_callback != NULL) {
            oneshot_callback(NULL);
            oneshot_callback = NULL;	// Double security for oneshot to happen once.
        }
    }
}
