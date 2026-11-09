#ifndef MAX7219_H
#define MAX7219_H

#include <stdint.h>

void MAX7219_Init(void);
void MAX7219_Clear(void);
void MAX7219_SetIntensity(uint8_t intensity);
void MAX7219_DisplayNumber(uint32_t number);

#endif