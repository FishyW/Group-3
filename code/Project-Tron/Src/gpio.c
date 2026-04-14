#include "gpio.h"

// Helper function to enable the peripheral clock for a given GPIO port
static void gpio_enable_clock(GPIO_TypeDef *port) {
    if (port == GPIOA) RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    else if (port == GPIOB) RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    else if (port == GPIOC) RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    else if (port == GPIOD) RCC->AHBENR |= RCC_AHBENR_GPIODEN;
    else if (port == GPIOE) RCC->AHBENR |= RCC_AHBENR_GPIOEEN;
    else if (port == GPIOF) RCC->AHBENR |= RCC_AHBENR_GPIOFEN;
}

gpio_status_t gpio_init(gpio_t *gpio, GPIO_TypeDef *port, uint8_t pin, gpio_mode_t mode) {
    // Check pin is valid for STM32 GPIO (0 to 15)
    if (pin > 15) return GPIO_ERROR_INVALID_PIN;

    // Save configuration in struct so the pin can be used later
    gpio->port = port;
    gpio->pin = pin;
    gpio->mode = mode;

    // Enable the RCC clock for this GPIO port
    gpio_enable_clock(port);

    // Clear the 2 MODER bits for this pin first
    port->MODER &= ~(0x3U << (2 * pin));

    // Set mode
    if (mode == GPIO_MODE_OUTPUT) {
        port->MODER |= (0x1U << (2 * pin));   // 01 = general purpose output
    } else if (mode == GPIO_MODE_INPUT) {
        // 00 = input, already cleared above
    } else {
        return GPIO_ERROR_INVALID_MODE;
    }

    return GPIO_OK;
}

gpio_status_t gpio_write(gpio_t *gpio, bool value) {
    // Only output pins should be written to
    if (gpio->mode != GPIO_MODE_OUTPUT) return GPIO_ERROR_INVALID_MODE;

    // Use BSRR register for atomic set/reset
    if (value) {
        gpio->port->BSRR = (1U << gpio->pin);          // set pin high
    } else {
        gpio->port->BSRR = (1U << (gpio->pin + 16));   // set pin low
    }

    return GPIO_OK;
}

bool gpio_read(gpio_t *gpio) {
    // Read pin state from input data register
    return ((gpio->port->IDR >> gpio->pin) & 1U);
}

void gpio_toggle(gpio_t *gpio) {
    // Toggle only makes sense for outputs
    if (gpio->mode != GPIO_MODE_OUTPUT) return;

    // Read current output state, then write opposite value
    if ((gpio->port->ODR >> gpio->pin) & 1U) {
        gpio->port->BSRR = (1U << (gpio->pin + 16));   // reset
    } else {
        gpio->port->BSRR = (1U << gpio->pin);          // set
    }
}
