// gpio.h
#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f303xc.h"

// GPIO direction/mode
typedef enum {
    GPIO_MODE_INPUT,
    GPIO_MODE_OUTPUT
} gpio_mode_t;

// Return status for basic error checking
typedef enum {
    GPIO_OK = 0,
    GPIO_ERROR_INVALID_PIN,
    GPIO_ERROR_INVALID_MODE
} gpio_status_t;

// Generic GPIO object
// Stores which port/pin is being used and whether it is input/output
typedef struct {
    GPIO_TypeDef *port;
    uint8_t pin;
    gpio_mode_t mode;
} gpio_t;

// Configure a GPIO pin for input or output
gpio_status_t gpio_init(gpio_t *gpio, GPIO_TypeDef *port, uint8_t pin, gpio_mode_t mode);

// Write a logic value to an output pin
gpio_status_t gpio_write(gpio_t *gpio, bool value);

// Read the logic value from a pin
bool gpio_read(gpio_t *gpio);

// Toggle the current output value
void gpio_toggle(gpio_t *gpio);

#endif
