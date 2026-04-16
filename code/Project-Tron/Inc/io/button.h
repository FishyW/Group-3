#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>

// Function pointer type for button callback
typedef void (*button_callback_t)(void);

// Initialise user button and register callback
void button_init(button_callback_t callback);

// Read button state directly
bool button_is_pressed(void);

#endif
