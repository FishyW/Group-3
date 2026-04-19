// integration file for board 1
#include "integration.h"

#include "io/button.h"
#include "io/led.h"

#include "comm/i2c.h"
#include "comm/uart.h"

#include "sensors/magnetometer.h"
#include "timer.h"

#include <stdint.h>

// stores current mode sent to Board B
// 0 = servo mode
// 1 = LED array mode
static volatile uint8_t button_mode = 0;

// update LEDs so exactly one is on for each state
static void updateModeLeds(void)
{
    if (button_mode == 0U) {
        // state 0
        led_set(LED0, true);
        led_set(LED1, false);
    } else {
        // state 1
        led_set(LED0, false);
        led_set(LED1, true);
    }
}

// interrupt-driven button callback
static void buttonPressed(void)
{
    // toggle mode each time button is pressed
    button_mode ^= 1U;

    // update LEDs to match new mode
    updateModeLeds();
}

static void initializeBoardA(void)
{
    // initialise local indicator LEDs first
    led_init();
    updateModeLeds();

    // initialise communication/sensor modules
    i2c1Init();
    serialInitialise(&USART1_PORT, BAUD_9600, 0x00);
    initElapsedTimer();
    magInit();

    // enable button interrupt callback last
    button_init(buttonPressed);
}

// Let there be 2 boards: Board A and Board B
// Board A sends over UART magnetometer data to Board B
// Board A also has an interrupt-driven function for the button
// The button tells Board B how to display the output
void testBoardA(void)
{
    BoardMessage boardMsg;

    initializeBoardA();

    while (1) {
        // Jack
        // read magnetometer sample into message struct
        if (magReadSample(&boardMsg.magSample) == 1) {

            // include button/display mode in the same message
            boardMsg.buttonMode = button_mode;

            // Winston
            // send <magData + button state> to UART
            sendMsg(&USART1_PORT, (uint8_t *) &boardMsg, sizeof(BoardMessage), 0);
        }

        // Denny
        // set the delay to 10 ms => 100 Hz
        // same as the magnetometer ODR
        delayElapsed(10);
    }
}
