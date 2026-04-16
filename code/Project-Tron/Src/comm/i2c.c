/**
 * Hardware mapping
 * ----------------
 *   PB6  -->  I2C1_SCL  (AF4, open-drain, pull-up)
 *   PB7  -->  I2C1_SDA  (AF4, open-drain, pull-up)
 *
 * Bus speed : 100 kHz
 * Clock     : 8 MHz
 *
 * Operation
 * ---------
 * 1. Configure PB6 and PB7 for I2C1 alternate function.
 * 2. Configure I2C1 timing for 100 kHz operation.
 * 3. Use blocking transfers with timeout protection.
 * 4. Support:
 *      - single-register write
 *      - single-register read
 *      - multi-byte read with optional auto-increment
 */

#include "stm32f303xc.h"
#include "comm/i2c.h"

/* tick_ms is defined in main.c and used here for timeouts */
extern volatile uint32_t tick_ms;

/* ---------------------------------------------------------------
 * i2cWaitBusyClear
 *
 * Wait until the bus is no longer busy.
 *
 * Returns:
 *   1  if BUSY clears before timeout
 *   0  if timeout occurs
 * --------------------------------------------------------------- */
static int i2cWaitBusyClear(void)
{
    uint32_t start = tick_ms;

    while (I2C1->ISR & I2C_ISR_BUSY)
    {
        if ((tick_ms - start) > I2C_TIMEOUT_MS)
        {
            return 0;
        }
    }

    return 1;
}

/* ---------------------------------------------------------------
 * i2cWaitTXIS
 *
 * Wait until the transmit register is ready for the next byte.
 *
 * Returns:
 *   1  if TXIS is set
 *   0  if NACK or timeout occurs
 * --------------------------------------------------------------- */
static int i2cWaitTXIS(void)
{
    uint32_t start = tick_ms;

    while ((I2C1->ISR & I2C_ISR_TXIS) == 0U)
    {
        if (I2C1->ISR & I2C_ISR_NACKF)
        {
            return 0;
        }

        if ((tick_ms - start) > I2C_TIMEOUT_MS)
        {
            return 0;
        }
    }

    return 1;
}

/* ---------------------------------------------------------------
 * i2cWaitTC
 *
 * Wait until the current transfer is complete.
 *
 * This is used between the write phase and the repeated-start read
 * phase in a register-read transaction.
 *
 * Returns:
 *   1  if TC is set
 *   0  if NACK or timeout occurs
 * --------------------------------------------------------------- */
static int i2cWaitTC(void)
{
    uint32_t start = tick_ms;

    while ((I2C1->ISR & I2C_ISR_TC) == 0U)
    {
        if (I2C1->ISR & I2C_ISR_NACKF)
        {
            return 0;
        }

        if ((tick_ms - start) > I2C_TIMEOUT_MS)
        {
            return 0;
        }
    }

    return 1;
}

/* ---------------------------------------------------------------
 * i2cWaitRXNE
 *
 * Wait until one received byte is available in RXDR.
 *
 * Returns:
 *   1  if RXNE is set
 *   0  if NACK or timeout occurs
 * --------------------------------------------------------------- */
static int i2cWaitRXNE(void)
{
    uint32_t start = tick_ms;

    while ((I2C1->ISR & I2C_ISR_RXNE) == 0U)
    {
        if (I2C1->ISR & I2C_ISR_NACKF)
        {
            return 0;
        }

        if ((tick_ms - start) > I2C_TIMEOUT_MS)
        {
            return 0;
        }
    }

    return 1;
}

/* ---------------------------------------------------------------
 * i2cWaitSTOP
 *
 * Wait until the STOP condition flag is set.
 *
 * Returns:
 *   1  if STOPF is set
 *   0  if timeout occurs
 * --------------------------------------------------------------- */
static int i2cWaitSTOP(void)
{
    uint32_t start = tick_ms;

    while ((I2C1->ISR & I2C_ISR_STOPF) == 0U)
    {
        if ((tick_ms - start) > I2C_TIMEOUT_MS)
        {
            return 0;
        }
    }

    return 1;
}

/* ---------------------------------------------------------------
 * i2cClearStopNack
 *
 * Clear the STOP and NACK flags so the next transfer can start
 * cleanly.
 * --------------------------------------------------------------- */
static void i2cClearStopNack(void)
{
    I2C1->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;
}

/* ---------------------------------------------------------------
 * i2c1Init
 *
 * Configure GPIOB pins PB6 and PB7 for I2C1 and enable the
 * peripheral for 100 kHz operation.
 *
 * Pin setup:
 *   PB6 -> I2C1_SCL
 *   PB7 -> I2C1_SDA
 *
 * Both pins are configured as:
 *   - alternate function mode
 *   - open-drain
 *   - pull-up enabled
 *   - high speed
 *   - AF4
 * --------------------------------------------------------------- */
void i2c1Init(void)
{
    /* Enable clocks for GPIOB and I2C1 */
    RCC->AHBENR  |= RCC_AHBENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* Reset I2C1 to a clean state */
    RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

    /* Set PB6 and PB7 to alternate function mode */
    GPIOB->MODER &= ~(GPIO_MODER_MODER6 | GPIO_MODER_MODER7);
    GPIOB->MODER |=  (GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1);

    /* Set PB6 and PB7 to open-drain */
    GPIOB->OTYPER |= (GPIO_OTYPER_OT_6 | GPIO_OTYPER_OT_7);

    /* Enable pull-up resistors on PB6 and PB7 */
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR6 | GPIO_PUPDR_PUPDR7);
    GPIOB->PUPDR |=  (GPIO_PUPDR_PUPDR6_0 | GPIO_PUPDR_PUPDR7_0);

    /* Set PB6 and PB7 to high speed */
    GPIOB->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR6 | GPIO_OSPEEDER_OSPEEDR7);

    /* Select AF4 for PB6 and PB7 */
    GPIOB->AFR[0] &= ~((0xFUL << GPIO_AFRL_AFRL6_Pos) |
                       (0xFUL << GPIO_AFRL_AFRL7_Pos));
    GPIOB->AFR[0] |=  ((4UL  << GPIO_AFRL_AFRL6_Pos) |
                       (4UL  << GPIO_AFRL_AFRL7_Pos));

    /* Disable I2C1 before writing TIMINGR */
    I2C1->CR1 &= ~I2C_CR1_PE;

    /* Set timing for 100 kHz operation at 8 MHz clock */
    I2C1->TIMINGR = I2C1_TIMING_100KHZ;

    /* Re-enable I2C1 */
    I2C1->CR1 = I2C_CR1_PE;
}

/* ---------------------------------------------------------------
 * i2cWriteReg
 *
 * Write one byte to one device register.
 *
 * Parameters:
 *   addr7  - 7-bit I2C device address
 *   reg    - register address
 *   data   - data byte to write
 *
 * Notes:
 *   - addr7 is a 7-bit address.
 *   - It is shifted left by 1 so it sits in the address field of
 *     I2C1->CR2.
 *
 * Returns:
 *   1  on success
 *   0  on failure
 * --------------------------------------------------------------- */
int i2cWriteReg(uint8_t addr7, uint8_t reg, uint8_t data)
{
    /* Wait for the bus to become free */
    if (!i2cWaitBusyClear())
    {
        return 0;
    }

    i2cClearStopNack();

    /* Configure transfer:
     * - move 7-bit address into CR2 address field
     * - send 2 bytes: register + data
     * - auto-generate STOP after last byte
     * - start transfer
     */
    I2C1->CR2 = ((uint32_t)addr7 << 1) |
                (2UL << I2C_CR2_NBYTES_Pos) |
                I2C_CR2_AUTOEND |
                I2C_CR2_START;

    /* Send register address */
    if (!i2cWaitTXIS())
    {
        goto fail;
    }
    I2C1->TXDR = reg;

    /* Send data byte */
    if (!i2cWaitTXIS())
    {
        goto fail;
    }
    I2C1->TXDR = data;

    /* Wait for STOP */
    if (!i2cWaitSTOP())
    {
        goto fail;
    }

    i2cClearStopNack();
    return 1;

fail:
    i2cClearStopNack();
    return 0;
}

/* ---------------------------------------------------------------
 * i2cReadRegs
 *
 * Read one or more bytes starting from a device register.
 *
 * Parameters:
 *   addr7    - 7-bit I2C device address
 *   reg      - first register to read from
 *   buf      - destination buffer
 *   len      - number of bytes to read
 *   auto_inc - if true, set bit 7 of the sub-address so the device
 *              auto-increments the register pointer
 *
 * Operation:
 *   1. Write the register address
 *   2. Wait for transfer complete
 *   3. Issue repeated START
 *   4. Read 'len' bytes
 *
 * Returns:
 *   1  on success
 *   0  on failure
 * --------------------------------------------------------------- */
int i2cReadRegs(uint8_t addr7, uint8_t reg,
                uint8_t *buf, uint8_t len, bool auto_inc)
{
    uint8_t i;
    uint8_t subaddr;

    /* Check inputs */
    if ((buf == 0) || (len == 0U))
    {
        return 0;
    }

    /* Wait for the bus to become free */
    if (!i2cWaitBusyClear())
    {
        return 0;
    }

    i2cClearStopNack();

    /* Prepare sub-address */
    subaddr = reg;

    /* Set bit 7 if the device uses auto-increment */
    if (auto_inc)
    {
        subaddr |= 0x80U;
    }

    /* Phase 1: write register address */
    I2C1->CR2 = ((uint32_t)addr7 << 1) |
                (1UL << I2C_CR2_NBYTES_Pos) |
                I2C_CR2_START;

    if (!i2cWaitTXIS())
    {
        goto fail;
    }
    I2C1->TXDR = subaddr;

    /* Wait until write phase completes */
    if (!i2cWaitTC())
    {
        goto fail;
    }

    /* Phase 2: repeated START for read */
    I2C1->CR2 = ((uint32_t)addr7 << 1) |
                ((uint32_t)len << I2C_CR2_NBYTES_Pos) |
                I2C_CR2_RD_WRN |
                I2C_CR2_AUTOEND |
                I2C_CR2_START;

    /* Read all requested bytes */
    for (i = 0; i < len; i++)
    {
        if (!i2cWaitRXNE())
        {
            goto fail;
        }

        buf[i] = (uint8_t)I2C1->RXDR;
    }

    /* Wait for STOP */
    if (!i2cWaitSTOP())
    {
        goto fail;
    }

    i2cClearStopNack();
    return 1;

fail:
    i2cClearStopNack();
    return 0;
}

/* ---------------------------------------------------------------
 * i2cReadReg
 *
 * Read one byte from one device register.
 *
 * Parameters:
 *   addr7  - 7-bit I2C device address
 *   reg    - register address
 *   val    - destination byte
 *
 * Returns:
 *   1  on success
 *   0  on failure
 * --------------------------------------------------------------- */
int i2cReadReg(uint8_t addr7, uint8_t reg, uint8_t *val)
{
    return i2cReadRegs(addr7, reg, val, 1U, false);
}
