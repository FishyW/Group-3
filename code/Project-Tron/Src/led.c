#include "led.h"
#include "gpio.h"

// Private GPIO objects for the 8 LEDs
static gpio_t leds[8];

// Private LED state variable
// This is encapsulated inside the module so other code cannot directly edit it
static uint8_t led_state = 0;

// Optional advanced state variables for rate-limited updates
static uint8_t requested_state = 0;
static uint32_t min_update_ms = 100;
static volatile bool led_update_pending = false;

void led_init(void) {
    // STM32F3 Discovery LEDs are typically on GPIOE pins 8 to 15
    for (int i = 0; i < 8; i++) {
        gpio_init(&leds[i], GPIOE, 8 + i, GPIO_MODE_OUTPUT);
    }

    // Start with all LEDs off
    led_state = 0;
    requested_state = 0;
    led_update_pending = false;
}

void led_set(led_id_t led, bool state) {
    // Ignore invalid LED numbers
    if (led > LED7) return;

    // BASIC VERSION:
    // Directly update hardware and stored state
    gpio_write(&leds[led], state);

    if (state) {
        led_state |= (1U << led);
    } else {
        led_state &= ~(1U << led);
    }

    // ADVANCED VERSION IDEA:
    // Instead of changing hardware immediately, comment out the direct write above
    // and only update requested_state here. A timer callback would later apply it.
    /*
    if (state) {
        requested_state |= (1U << led);
    } else {
        requested_state &= ~(1U << led);
    }

    led_update_pending = true;
    */
}

bool led_get(led_id_t led) {
    // Return false for invalid LED number
    if (led > LED7) return false;

    // Read from the private stored state
    return ((led_state >> led) & 1U);
}

uint8_t led_get_all(void) {
    // Return complete LED state bitmask
    return led_state;
}

void led_set_all(uint8_t mask) {
    // Set all LEDs according to the bit pattern in mask
    for (int i = 0; i < 8; i++) {
        led_set((led_id_t)i, (mask >> i) & 1U);
    }
}

// Example of a timer-driven update function for the advanced task
// This would be called periodically by a timer module
void led_timer_callback(void) {
    static uint32_t elapsed_ms = 0;
    elapsed_ms++;

    // Only allow physical LED change every min_update_ms
    if (elapsed_ms >= min_update_ms && led_update_pending) {
        elapsed_ms = 0;
        led_update_pending = false;

        // Apply requested state to actual hardware
        for (int i = 0; i < 8; i++) {
            bool bit = (requested_state >> i) & 1U;
            gpio_write(&leds[i], bit);
        }

        // Update stored state after hardware output is changed
        led_state = requested_state;
    }
}
