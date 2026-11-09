#ifndef SPI_H
#define SPI_H

#include <stdint.h>

void SPI1_Init(void);
uint8_t SPI1_TransferByte(uint8_t data);

#endif