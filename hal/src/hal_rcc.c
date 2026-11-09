#include <stdint.h>
#include "hal_rcc.h"

#define RCC_BASE 0x40021000UL

typedef struct
{
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *)RCC_BASE)

/* ================= Hàm tổng quát ================= */

void RCC_EnableAHBClock(uint32_t peripherals)
{
    RCC->AHBENR |= peripherals;
}

void RCC_EnableAPB1Clock(uint32_t peripherals)
{
    RCC->APB1ENR |= peripherals;
}

void RCC_EnableAPB2Clock(uint32_t peripherals)
{
    RCC->APB2ENR |= peripherals;
}

void RCC_DisableAHBClock(uint32_t peripherals)
{
    RCC->AHBENR &= ~peripherals;
}

void RCC_DisableAPB1Clock(uint32_t peripherals)
{
    RCC->APB1ENR &= ~peripherals;
}

void RCC_DisableAPB2Clock(uint32_t peripherals)
{
    RCC->APB2ENR &= ~peripherals;
}

/* ================= Hàm tiện dụng ================= */

void RCC_EnableGPIOAClock(void)
{
    RCC_EnableAPB2Clock(RCC_APB2_GPIOA);
}

void RCC_EnableGPIOBClock(void)
{
    RCC_EnableAPB2Clock(RCC_APB2_GPIOB);
}

void RCC_EnableGPIOCClock(void)
{
    RCC_EnableAPB2Clock(RCC_APB2_GPIOC);
}

void RCC_EnableUSART1Clock(void)
{
    RCC_EnableAPB2Clock(RCC_APB2_USART1);
}

void RCC_EnableUSART2Clock(void)
{
    RCC_EnableAPB1Clock(RCC_APB1_USART2);
}

void RCC_EnableTIM2Clock(void)
{
    RCC_EnableAPB1Clock(RCC_APB1_TIM2);
}

void RCC_EnableTIM3Clock(void)
{
    RCC_EnableAPB1Clock(RCC_APB1_TIM3);
}