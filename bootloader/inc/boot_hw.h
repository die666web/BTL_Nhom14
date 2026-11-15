#ifndef BOOT_HW_H
#define BOOT_HW_H
/* Thanh ghi STM32F103 dùng trong bootloader (RM0008, PM0075, PM0056). Chỉ file target include. */
#include <stdint.h>

#define REG32(a) (*(volatile uint32_t *)(a))

/* RCC */
#define RCC_BASE        0x40021000u
#define RCC_APB2RSTR    REG32(RCC_BASE + 0x0Cu)
#define RCC_APB2ENR     REG32(RCC_BASE + 0x18u)
#define RCC_APB1ENR     REG32(RCC_BASE + 0x1Cu)
#define RCC_APB2_IOPA   (1u << 2)
#define RCC_APB2_IOPB   (1u << 3)
#define RCC_APB2_IOPC   (1u << 4)
#define RCC_APB2_USART1 (1u << 14)
#define RCC_APB1_BKP    (1u << 27)
#define RCC_APB1_PWR    (1u << 28)

/* PWR / BKP */
#define PWR_CR          REG32(0x40007000u)
#define PWR_CR_DBP      (1u << 8)
#define BKP_DR1         REG32(0x40006C04u)

/* GPIO */
#define GPIOA_BASE      0x40010800u
#define GPIOB_BASE      0x40010C00u
#define GPIOC_BASE      0x40011000u
#define GPIO_CRL(b)     REG32((b) + 0x00u)
#define GPIO_CRH(b)     REG32((b) + 0x04u)
#define GPIO_BSRR(b)    REG32((b) + 0x10u)

/* USART1 */
#define USART1_BASE     0x40013800u
#define USART1_SR       REG32(USART1_BASE + 0x00u)
#define USART1_DR       REG32(USART1_BASE + 0x04u)
#define USART1_BRR      REG32(USART1_BASE + 0x08u)
#define USART1_CR1      REG32(USART1_BASE + 0x0Cu)
#define USART1_CR2      REG32(USART1_BASE + 0x10u)
#define USART1_CR3      REG32(USART1_BASE + 0x14u)
#define USART_SR_PE     (1u << 0)
#define USART_SR_FE     (1u << 1)
#define USART_SR_NE     (1u << 2)
#define USART_SR_ORE    (1u << 3)
#define USART_SR_RXNE   (1u << 5)
#define USART_SR_TC     (1u << 6)
#define USART_SR_TXE    (1u << 7)
#define USART_CR1_RE    (1u << 2)
#define USART_CR1_TE    (1u << 3)
#define USART_CR1_UE    (1u << 13)

/* FLASH */
#define FLASH_BASE_R    0x40022000u
#define FLASH_KEYR      REG32(FLASH_BASE_R + 0x04u)
#define FLASH_SR        REG32(FLASH_BASE_R + 0x0Cu)
#define FLASH_CR        REG32(FLASH_BASE_R + 0x10u)
#define FLASH_AR        REG32(FLASH_BASE_R + 0x14u)
#define FLASH_KEY1      0x45670123u
#define FLASH_KEY2      0xCDEF89ABu
#define FLASH_SR_BSY    (1u << 0)
#define FLASH_SR_PGERR  (1u << 2)
#define FLASH_SR_WRPRT  (1u << 4)
#define FLASH_SR_EOP    (1u << 5)
#define FLASH_CR_PG     (1u << 0)
#define FLASH_CR_PER    (1u << 1)
#define FLASH_CR_STRT   (1u << 6)
#define FLASH_CR_LOCK   (1u << 7)

/* Cortex-M3 */
#define SYST_CSR        REG32(0xE000E010u)
#define SYST_RVR        REG32(0xE000E014u)
#define SYST_CVR        REG32(0xE000E018u)
#define NVIC_ICER(n)    REG32(0xE000E180u + 4u * (n))
#define NVIC_ICPR(n)    REG32(0xE000E280u + 4u * (n))
#define SCB_ICSR        REG32(0xE000ED04u)
#define SCB_VTOR        REG32(0xE000ED08u)
#define SCB_ICSR_PENDSTCLR (1u << 25)

#endif
