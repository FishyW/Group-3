#include "io/button.h"
#include "io/gpio.h"
#include "stm32f303xc.h"

// Private GPIO object for the button
static gpio_t button_gpio;

// Private function pointer storing the callback
static button_callback_t button_cb = 0;

void button_init(button_callback_t callback) {
    // Save callback function pointer for later use
    button_cb = callback;

    // Discovery board user button is typically PA0
    gpio_init(&button_gpio, GPIOA, 0, GPIO_MODE_INPUT);

    // Enable SYSCFG clock for EXTI configuration
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // Connect EXTI0 line to PA0
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;

    // Unmask EXTI line 0
    EXTI->IMR |= EXTI_IMR_MR0;

    // Trigger interrupt on rising edge
    EXTI->RTSR |= EXTI_RTSR_TR0;

    // Enable EXTI0 interrupt in NVIC
    NVIC_EnableIRQ(EXTI0_IRQn);
}

bool button_is_pressed(void) {
    // Return current logic level on the button pin
    return gpio_read(&button_gpio);
}

// Interrupt service routine for EXTI line 0
void EXTI0_IRQHandler(void) {
    // Check interrupt came from EXTI0
    if (EXTI->PR & EXTI_PR_PR0) {
        // Clear pending flag
        EXTI->PR = EXTI_PR_PR0;

        // Call the registered callback if it exists
        if (button_cb != 0) {
            button_cb();
        }
    }
}
