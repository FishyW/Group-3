#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include <stdint.h>

// Logical LED numbering for the 8 Discovery board LEDs
typedef enum {
    LED0 = 0,
    LED1,
    LED2,
    LED3,
    LED4,
    LED5,
    LED6,
    LED7
} led_id_t;

// Initialise all Discovery board LEDs
void led_init(void);

// Request/set one LED state
void led_set(led_id_t led, bool state);

// Read back the stored software state of one LED
bool led_get(led_id_t led);

// Read back the stored state of all LEDs as a bitmask
uint8_t led_get_all(void);

// Set all LEDs using a bitmask
void led_set_all(uint8_t mask);


// Test Functions
void testLedSpeedLimiter();

#endif
