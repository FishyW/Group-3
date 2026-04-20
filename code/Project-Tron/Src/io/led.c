#include "io/led.h"
#include "io/gpio.h"

#include "timer.h"


// Private GPIO objects for the 8 LEDs
static gpio_t leds[8];

// Private LED state variable
// This is encapsulated inside the module so other code cannot directly edit it
static uint8_t led_state = 0;

// Optional advanced state variables for rate-limited updates
static uint8_t requested_state = 0;
static uint32_t min_update_ms = 100;
static volatile bool led_update_pending = false;


uint32_t start_time_led[8] = {};
uint32_t rate_limit_led_us = 0;


void led_init(void) {

	// init elapsed timer
	initElapsedTimer();

	// init led times
	for (uint8_t i = 0; i < 8; ++i) {
		start_time_led[i] = getNow();
	}


    // STM32F3 Discovery LEDs are typically on GPIOE pins 8 to 15
    for (uint8_t i = 0; i < 8; i++) {
        gpio_init(&leds[i], GPIOE, 8 + i, GPIO_MODE_OUTPUT);
    }

    // Start with all LEDs off
    led_state = 0;
    requested_state = 0;
    led_update_pending = false;
}

void led_set_rate_limit(uint32_t limit_us) {
	rate_limit_led_us = limit_us;
}


void led_set(led_id_t led, bool state) {

    // Ignore invalid LED numbers
    if (led > LED7) return;

    // restrict led set speed
   if (getNow() - start_time_led[led] < rate_limit_led_us) {
	   return;
   }

   start_time_led[led] = getNow();

    // BASIC VERSION:
    // Directly update hardware and stored state
    gpio_write(&leds[led], state);

    if (state) {
        led_state |= (1U << led);
    } else {
        led_state &= ~(1U << led);
    }

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



void testLedSpeedLimiter() {
	led_init();

	// set rate limit to 1 second
	led_set_rate_limit(1000 * 1000);

	uint8_t counter = 0;
	for (;;) {
		led_set_all(counter);
		counter += 1;
		delayElapsed(100);
	}
}
