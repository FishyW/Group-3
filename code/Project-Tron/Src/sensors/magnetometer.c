#include <math.h>
#include "sensors/magnetometer.h"
#include "comm/i2c.h"
#include "comm/uart.h"

extern volatile uint32_t tick_ms;

/* PI used for the heading conversion from radians to degrees */

/* ---------------------------------------------------------------
 * magInit
 *
 * Configures the LSM303AGR magnetometer for continuous-conversion
 * mode at 10 Hz with block-data-update enabled.
 *
 * Register values written:
 *   CFG_REG_A_M = 0x80  --> temp comp on, high-res, 10 Hz, continuous
 *   CFG_REG_B_M = 0x00  --> default (offset cancellation off)
 *   CFG_REG_C_M = 0x10  --> BDU enabled (registers hold until both
 *                            H and L bytes have been read)
 *
 * Returns:
 *    1  on success
 *    0  if any I2C write fails
 * --------------------------------------------------------------- */
int magInit(void)
{
    /* ---- CFG_REG_A_M = 0x80 ----
     * Bit 7   : TEMP_COMP_EN = 1  --> temperature compensation enabled
     * Bits 4:2: ODR / LP settings
     * Bits 1:0: MD = 00           --> continuous-conversion mode
     */
	// ODR = 11 -> 100Hz (same frequency as accelerometer)
    if (!i2cWriteReg(MAG_ADDR_7BIT, AGR_CFG_REG_A_M, 0b10001100))
        return 0;

    /* ---- CFG_REG_B_M = 0x00 ----
     */
    // LPF = 1 (enable low pass filter): bit 0
    // OFF_CANC = 1 (set offset cancellation to prevent drift = 1): bit 1
    if (!i2cWriteReg(MAG_ADDR_7BIT, AGR_CFG_REG_B_M, 0b00000011))
        return 0;

    /* ---- CFG_REG_C_M = 0x10 ----
     * Bit 4 : BDU = 1 --> block data update: output registers are
     *                     not updated until both H and L have been read.
     *                     Prevents mixing of old H and new L bytes.
     */
    if (!i2cWriteReg(MAG_ADDR_7BIT, AGR_CFG_REG_C_M, 0b00010000))
        return 0;

    return 1;
}

/* ---------------------------------------------------------------
 * magHeadingDeg  (private helper)
 *
 * Computes the compass heading in degrees [-180, 180) from raw X and Y
 * axis counts using the two-argument arctangent.
 *
 * 0 corresponds to North, 90 corresponds to East
 * Due to electromagnetic disturbance, the heading might be inaccurate
 * Thus it is recommended to position the STM32 far away from any electronic device
 *
 * atan2f returns a value in (-pi, +pi] radians.
 * Negative results are shifted up by 360 degrees.
 *
 * Note: this is only a horizontal-plane heading calculation.
 * If the board is tilted, the result will drift.
 * --------------------------------------------------------------- */
static float magHeadingDeg(int16_t x, int16_t y)
{
    float heading = atan2f((float)x, (float)y) * (180.0f / M_PI);

    return heading;
}

/* ---------------------------------------------------------------
 * magReadSample
 *
 * Checks the data-ready flag (ZYXDA bit in STATUS_REG_M) and, if a
 * new sample is available, reads the six output registers in a single
 * burst and stores the results in *s.
 *
 * The LSM303AGR stores data in little-endian order (L byte first):
 *   OUTX_L, OUTX_H, OUTY_L, OUTY_H, OUTZ_L, OUTZ_H
 *
 * After reading, heading_deg is computed using atan2(Y, X) and
 * timestamp_ms is captured from the global tick_ms counter.
 *
 * Parameters:
 *   s  - pointer to a MagSample structure to fill
 *
 * Returns:
 *    1  - new sample stored in *s  (s->valid is set to 1)
 *    0  - no new data ready yet    (s is unchanged)
 *   -1  - I2C error or null pointer
 * --------------------------------------------------------------- */
int magReadSample(MagSample *s)
{
    uint8_t sr  = 0;
    uint8_t raw[6];

    if (s == 0)
        return -1;

    /* ---- Check the data-ready flag (ZYXDA = bit 3 of STATUS_REG_M) ----
     * If this bit is 0, no new measurement is available yet.
     * Return 0 to tell the caller to try again later.
     */
    if (!i2cReadReg(MAG_ADDR_7BIT, AGR_STATUS_REG_M, &sr))
        return -1;

    if ((sr & 0x08U) == 0U)
        return 0;   /* no new data ready */

    /* ---- Burst-read all six output registers ----
     * auto_inc = true: bit 7 of the sub-address is set, so the device
     * auto-increments the register pointer after each byte.
     * Registers read: OUTX_L(0x68), OUTX_H, OUTY_L, OUTY_H, OUTZ_L, OUTZ_H
     */
    if (!i2cReadRegs(MAG_ADDR_7BIT, AGR_OUTX_L_REG_M, raw, 6U, true))
        return -1;

    /* ---- Reconstruct 16-bit values from little-endian byte pairs ----
     * LSM303AGR stores L byte at lower address, H byte at higher address.
     * The cast to int16_t sign-extends the result correctly.
     */

    // From application note, multiply by 1.5
    s->raw_x = (int16_t)((raw[1] << 8) | raw[0]) * 1.5;   /* X: raw[1]=H, raw[0]=L */
    s->raw_y = (int16_t)((raw[3] << 8) | raw[2]) * 1.5;   /* Y: raw[3]=H, raw[2]=L */
    s->raw_z = (int16_t)((raw[5] << 8) | raw[4]) * 1.5;   /* Z: raw[5]=H, raw[4]=L */

    /* ---- Compute heading and record timestamp ---- */
    s->heading_deg  = magHeadingDeg(s->raw_x, s->raw_y);
    s->timestamp_ms = tick_ms;   /* capture current time from system tick */
    s->valid        = 1U;

    return 1;
}

/* ---------------------------------------------------------------
 * magPrintSample
 *
 * Sends a human-readable representation of *s over UART in the form:
 *
 *   T=12345ms  X=  123  Y= -456  Z=  789  Heading=273.12 deg
 *
 * Parameters:
 *   s  - pointer to a MagSample to print (must be valid)
 * --------------------------------------------------------------- */
void magPrintSample(const MagSample *s)
{
//    if ((s == 0) || (s->valid == 0U))
//        return;
//
//    /* Print timestamp first so each line is self-contained */
//    uartSendString("T=");
//    uartSendUInt(s->timestamp_ms);
//    uartSendString("ms  ");
//
//    /* Raw axis values */
//    uartSendString("X=");
//    uartSendInt(s->raw_x);
//    uartSendString("  Y=");
//    uartSendInt(s->raw_y);
//    uartSendString("  Z=");
//    uartSendInt(s->raw_z);
//
//    /* Decoded heading */
//    uartSendString("  Heading=");
//    uartSendFixed2(s->heading_deg);
//    uartSendString(" deg\r\n");
}
