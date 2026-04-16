#ifndef MAGNETOMETER_H
#define MAGNETOMETER_H

#include <stdint.h>

/**
 * magnetometer.h
 *
 * Public API for the LSM303AGR magnetometer driver.
 *
 * This module provides:
 *   - register definitions used by the driver
 *   - a MagSample structure for one reading
 *   - functions to initialise, read, and print magnetometer data
 */

/* 7-bit I2C address of the magnetometer */
#define MAG_ADDR_7BIT    0x1EU

/* Magnetometer registers used by this driver */
#define AGR_CFG_REG_A_M  0x60U
#define AGR_CFG_REG_B_M  0x61U
#define AGR_CFG_REG_C_M  0x62U
#define AGR_STATUS_REG_M 0x67U
#define AGR_OUTX_L_REG_M 0x68U

/* One magnetometer sample */
typedef struct
{
    int16_t  raw_x;         /* raw X reading */
    int16_t  raw_y;         /* raw Y reading */
    int16_t  raw_z;         /* raw Z reading */
    float    heading_deg;   /* heading in degrees, 0 to 360 */
    uint32_t timestamp_ms;  /* time sample was read */
    uint8_t  valid;         /* 1 = valid sample, 0 = invalid */
} MagSample;

/* ---------------------------------------------------------------
 * magInit
 *
 * Initialises the magnetometer for continuous-conversion mode.
 *
 * Returns:
 *   1  on success
 *   0  on failure
 * --------------------------------------------------------------- */
int magInit(void);

/* ---------------------------------------------------------------
 * magReadSample
 *
 * Reads one magnetometer sample if new data is available.
 *
 * Parameters:
 *   s  - pointer to destination sample structure
 *
 * Returns:
 *   1  if a new sample was read
 *   0  if no new data is ready yet
 *  -1  on error
 * --------------------------------------------------------------- */
int magReadSample(MagSample *s);

/* ---------------------------------------------------------------
 * magPrintSample
 *
 * Prints a valid sample over UART.
 *
 * Parameters:
 *   s  - pointer to sample to print
 * --------------------------------------------------------------- */
void magPrintSample(const MagSample *s);

#endif /* MAGNETOMETER_H */
