#include <math.h>
#include <stdint.h>

#include "sensors/magnetometer.h"
#include "comm/i2c.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern volatile uint32_t tick_ms;



/* ---------------------------------------------------------------
 * magInit
 *
 * Configures the LSM303AGR magnetometer for continuous-conversion
 * mode at 100 Hz with low-pass filter, offset cancellation, and
 * block-data-update enabled.
 *
 * Register values written:
 *   CFG_REG_A_M = 0b10001100  --> temp comp on, 100 Hz, continuous
 *   CFG_REG_B_M = 0b00000011  --> LPF on, offset cancellation on
 *   CFG_REG_C_M = 0b00010000  --> BDU enabled
 *
 * Returns:
 *    1  on success
 *    0  if any I2C write fails
 * --------------------------------------------------------------- */
int magInit(void)
{
    /* CFG_REG_A_M
     * Bit 7   : TEMP_COMP_EN = 1
     * Bits 4:2: ODR = 100 Hz
     * Bits 1:0: MD = 00 (continuous-conversion mode)
     */
    if (!i2cWriteReg(MAG_ADDR_7BIT, AGR_CFG_REG_A_M, 0b10001100))
        return 0;

    /* CFG_REG_B_M
     * Bit 0 : LPF = 1
     * Bit 1 : OFF_CANC = 1
     */
    if (!i2cWriteReg(MAG_ADDR_7BIT, AGR_CFG_REG_B_M, 0b00000011))
        return 0;

    /* CFG_REG_C_M
     * Bit 4 : BDU = 1
     */
    if (!i2cWriteReg(MAG_ADDR_7BIT, AGR_CFG_REG_C_M, 0b00010000))
        return 0;

    return 1;
}

/* ---------------------------------------------------------------
 * magHeadingDeg
 *
 * Computes heading in degrees in the range [-180, 180] from the
 * raw X and Y axes using atan2f.
 * --------------------------------------------------------------- */
static float magHeadingDeg(int16_t x, int16_t y)
{
    float heading = atan2f((float)x, (float)y) * (180.0f / (float)M_PI);
    return heading;
}

/* ---------------------------------------------------------------
 * magReadSample
 *
 * Checks the data-ready flag and, if a new sample is available,
 * reads all 3 axes and stores the results in *s.
 *
 * Returns:
 *    1  new sample stored
 *    0  no new data ready
 *   -1  I2C error or null pointer
 * --------------------------------------------------------------- */
int magReadSample(MagSample *s)
{
    uint8_t sr = 0;
    uint8_t raw[6];

    int16_t x_raw;
    int16_t y_raw;
    int16_t z_raw;

    if (s == 0)
        return -1;

    if (!i2cReadReg(MAG_ADDR_7BIT, AGR_STATUS_REG_M, &sr))
        return -1;

    /* bit 3 = ZYXDA, new XYZ data available */
    if ((sr & 0x08U) == 0U)
        return 0;

    if (!i2cReadRegs(MAG_ADDR_7BIT, AGR_OUTX_L_REG_M, raw, 6U, true))
        return -1;

    /* Reconstruct signed 16-bit little-endian values */
    x_raw = (int16_t)(((uint16_t)raw[1] << 8) | raw[0]);
    y_raw = (int16_t)(((uint16_t)raw[3] << 8) | raw[2]);
    z_raw = (int16_t)(((uint16_t)raw[5] << 8) | raw[4]);

    /* Keep your 1.5 mG/LSB scaling */
    s->raw_x = (int16_t)(x_raw * 1.5f);
    s->raw_y = (int16_t)(y_raw * 1.5f);
    s->raw_z = (int16_t)(z_raw * 1.5f);

    s->heading_deg  = magHeadingDeg(s->raw_x, s->raw_y);
    s->timestamp_ms = tick_ms;
    s->valid        = 1U;

    return 1;
}

/* ---------------------------------------------------------------
 * magPrintSample
 *
 * Not used in the current integration path.
 * Kept here so the public interface still matches the header.
 * --------------------------------------------------------------- */
void magPrintSample(const MagSample *s)
{
    (void)s;
}
