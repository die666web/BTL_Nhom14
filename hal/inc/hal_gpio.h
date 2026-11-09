/* GPIO_ConfigPin: init before scheduler only. Pin writes atomic via BSRR/BRR.
 * One owner per output; relay belongs to TaskControl. */
#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#define GPIOA_BASE 0x40010800UL
#define GPIOB_BASE 0x40010C00UL
#define GPIOC_BASE 0x40011000UL

typedef struct
{
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)

typedef enum
{
    GPIO_MODE_ANALOG,
    GPIO_MODE_INPUT_FLOATING,
    GPIO_MODE_INPUT_PULLUP,
    GPIO_MODE_INPUT_PULLDOWN,
    GPIO_MODE_OUTPUT_PP,
    GPIO_MODE_OUTPUT_OD,
    GPIO_MODE_AF_PP,
    GPIO_MODE_AF_OD
} GPIO_Mode;

typedef enum
{
    GPIO_SPEED_INPUT = 0,
    GPIO_SPEED_10MHZ = 1,
    GPIO_SPEED_2MHZ  = 2,
    GPIO_SPEED_50MHZ = 3
} GPIO_Speed;

void GPIO_ConfigPin(GPIO_TypeDef *GPIOx, uint8_t pin,
                    GPIO_Mode mode, GPIO_Speed speed);
void GPIO_WritePin(GPIO_TypeDef *GPIOx, uint8_t pin, uint8_t state);
uint8_t GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint8_t pin);

#endif
