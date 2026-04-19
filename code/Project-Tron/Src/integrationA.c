// integration file for board 1
#include "integration.h"

#include "io/button.h"
#include "io/led.h"

#include "comm/i2c.h"
#include "comm/uart.h"

#include "sensors/magnetometer.h"
#include "timer.h"

#include <stdint.h>

// stores current display state sent to Board B
// 0 = servo mode
// 1 = LED array mode
static volatile uint8_t displayState = 0;

// update LEDs so exactly one is on for each state
static void updateModeLeds(void)
{
    if (displayState == 0U) {
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
    // toggle display state each time button is pressed
    displayState ^= 1U;

    // update LEDs to match new state
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
        // read magnetometer sample into message struct
        if (magReadSample(&boardMsg.magSample) == 1) {

            // include display state in the same message
            boardMsg.displayState = displayState;

            // send <magData + displayState> to UART
            sendMsg(&USART1_PORT, (uint8_t *)&boardMsg, sizeof(BoardMessage), 0);
        }

        // 10 ms => 100 Hz
        delayElapsed(10);
    }
}
