#include "stm32f303xc.h"
#include "i2c.h"
#include "uart.h"
#include "magnetometer.h"

#define CPU_CLK_HZ 8000000UL

/* global millisecond counter */
volatile uint32_t tick_ms = 0;

/* latest sample storage */
static volatile MagSample g_mag = {0};

/* delay for a given number of milliseconds */
void delayMs(uint32_t ms)
{
    uint32_t start = tick_ms;

    while ((tick_ms - start) < ms)
    {
    }
}

/* SysTick interrupt runs every 1 ms */
void SysTick_Handler(void)
{
    tick_ms++;
}

int main(void)
{
    int rc;

    /* enable FPU */
    SCB->CPACR |= (0xFUL << 20);
    __DSB();
    __ISB();

    /* set SysTick to 1 ms period */
    SysTick_Config(CPU_CLK_HZ / 1000UL);

    /* initialise peripherals */
    uartInit();
    i2c1Init();

    uartSendString("\r\n=== STM32F3Discovery MAGNETOMETER READINGS ===\r\n");

    if (!magInit())
    {
        uartSendString("ERROR: Magnetometer init failed\r\n");

        while (1)
        {
            delayMs(1000);
        }
    }

    /* print this before any sample output */
    uartSendString("Magnetometer initialised. Reading samples...\r\n\r\n");

    while (1)
    {
        /* magReadSample return values:
         *   1  = new sample read
         *   0  = no new data yet
         *  -1  = read error
         */
        rc = magReadSample((MagSample *)&g_mag);

        if (rc == 1)
        {
            /* print fresh sample */
            magPrintSample((const MagSample *)&g_mag);
        }
        else if (rc == 0)
        {
            /* no new data yet */
            uartSendString("No new data\r\n");
        }
        else
        {
            /* read failed */
            uartSendString("ERROR: Read failed\r\n");
        }

        /* poll at about 1 Hz */
        delayMs(1000);
    }
}
