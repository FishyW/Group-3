#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Bus speed : 100 kHz
 * Clock     : 8 MHz
 *
 * All functions are blocking and use a timeout so the code does not
 * get stuck forever if the bus hangs or a device does not respond.
 */

/* ---------------------------------------------------------------
 * I2C1_TIMING_100KHZ
 *
 * Timing value for 100 kHz I2C operation with an 8 MHz clock.
 * --------------------------------------------------------------- */
#define I2C1_TIMING_100KHZ  0x2000090EUL

/* ---------------------------------------------------------------
 * I2C_TIMEOUT_MS
 *
 * Maximum wait time in milliseconds for any blocking I2C step.
 * --------------------------------------------------------------- */
#define I2C_TIMEOUT_MS      50U

/* ---------------------------------------------------------------
 * i2c1Init
 *
 * Configures PB6 and PB7 for I2C1 and enables the peripheral.
 *
 * Must be called before any read or write functions.
 * --------------------------------------------------------------- */
void i2c1Init(void);

/* ---------------------------------------------------------------
 * i2cWriteReg
 *
 * Writes one byte to one register.
 *
 * Parameters:
 *   addr7  - 7-bit device address
 *   reg    - register address
 *   data   - byte to write
 *
 * Returns:
 *   1  on success
 *   0  on failure
 * --------------------------------------------------------------- */
int i2cWriteReg(uint8_t addr7, uint8_t reg, uint8_t data);

/* ---------------------------------------------------------------
 * i2cReadRegs
 *
 * Reads one or more bytes starting at a register address.
 *
 * Parameters:
 *   addr7    - 7-bit device address
 *   reg      - first register to read
 *   buf      - destination buffer
 *   len      - number of bytes to read
 *   auto_inc - if true, enable register auto-increment
 *
 * Returns:
 *   1  on success
 *   0  on failure
 * --------------------------------------------------------------- */
int i2cReadRegs(uint8_t addr7, uint8_t reg,
                uint8_t *buf, uint8_t len, bool auto_inc);

/* ---------------------------------------------------------------
 * i2cReadReg
 *
 * Reads one byte from one register.
 *
 * Parameters:
 *   addr7  - 7-bit device address
 *   reg    - register address
 *   val    - pointer to destination byte
 *
 * Returns:
 *   1  on success
 *   0  on failure
 * --------------------------------------------------------------- */
int i2cReadReg(uint8_t addr7, uint8_t reg, uint8_t *val);

#endif /* I2C_H */
