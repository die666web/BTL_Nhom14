/* Clock enable/disable: BSP/init before scheduler only; not callable from tasks.
 * These are register primitives, not a shared runtime clock manager. */
#ifndef RCC_H
#define RCC_H

#include <stdint.h>

/* ==================== AHB ==================== */

#define RCC_AHB_DMA1       (1U << 0)
#define RCC_AHB_SRAM       (1U << 2)
#define RCC_AHB_CRC        (1U << 6)

/* ==================== APB2 =================== */

#define RCC_APB2_AFIO      (1U << 0)

#define RCC_APB2_GPIOA     (1U << 2)
#define RCC_APB2_GPIOB     (1U << 3)
#define RCC_APB2_GPIOC     (1U << 4)
#define RCC_APB2_GPIOD     (1U << 5)

#define RCC_APB2_ADC1      (1U << 9)
#define RCC_APB2_TIM1      (1U << 11)
#define RCC_APB2_SPI1      (1U << 12)
#define RCC_APB2_USART1    (1U << 14)

/* ==================== APB1 =================== */

#define RCC_APB1_TIM2      (1U << 0)
#define RCC_APB1_TIM3      (1U << 1)
#define RCC_APB1_TIM4      (1U << 2)

#define RCC_APB1_SPI2      (1U << 14)

#define RCC_APB1_USART2    (1U << 17)
#define RCC_APB1_USART3    (1U << 18)

#define RCC_APB1_I2C1      (1U << 21)
#define RCC_APB1_I2C2      (1U << 22)

#define RCC_APB1_USB       (1U << 23)
#define RCC_APB1_CAN       (1U << 25)
#define RCC_APB1_PWR       (1U << 28)

/* Hàm tổng quát */

void RCC_EnableAHBClock(uint32_t peripherals);
void RCC_EnableAPB1Clock(uint32_t peripherals);
void RCC_EnableAPB2Clock(uint32_t peripherals);

void RCC_DisableAHBClock(uint32_t peripherals);
void RCC_DisableAPB1Clock(uint32_t peripherals);
void RCC_DisableAPB2Clock(uint32_t peripherals);

/* Một số hàm tiện dụng */

void RCC_EnableGPIOAClock(void);
void RCC_EnableGPIOBClock(void);
void RCC_EnableGPIOCClock(void);

void RCC_EnableUSART1Clock(void);
void RCC_EnableUSART2Clock(void);

void RCC_EnableTIM2Clock(void);
void RCC_EnableTIM3Clock(void);

#endif