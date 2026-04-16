/**
 * SPI Header File to interface with
 * ST MEMS I3G4250D/L3GD20 gyroscope
 */

#ifndef SPI_H
#define SPI_H

#include <stdint.h>

void initializeSPI();


uint8_t readSPIRegister(uint8_t address);

void writeSPIRegister(uint8_t address, uint8_t value);




#endif
