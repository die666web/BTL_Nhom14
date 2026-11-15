#include "bsp.h"
#include "board_config.h"
#include "hal_gpio.h"
#include "hal_rcc.h"
#include "ota_layout.h"
#include <stdint.h>

#define BSP_RCC_APB1ENR  (*(volatile uint32_t *)0x4002101CUL)
#define BSP_PWR_CR       (*(volatile uint32_t *)0x40007000UL)
#define BSP_BKP_DR1      (*(volatile uint32_t *)0x40006C04UL)
#define BSP_SCB_AIRCR    (*(volatile uint32_t *)0xE000ED0CUL)

void bsp_relay_force_off(void)
{
    GPIO_WritePin(GPIOB, BSP_RELAY_PIN, !BSP_RELAY_ACTIVE);
}

void bsp_system_init(void)
{
    /* App chạy sau bootloader (0x08002000). Bootloader để lại: HSI 8 MHz, VTOR = 0x08002000,
     * SysTick/IRQ tắt, USART1/GPIOA/GPIOC đã reset, PB8 (relay) đang ở mức OFF. */
    RCC_EnableGPIOAClock();
    RCC_EnableGPIOBClock();
    /* Preload inactive level before switching PB8 to output. */
    bsp_relay_force_off();
    GPIO_ConfigPin(GPIOB, BSP_RELAY_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_2MHZ);
    GPIO_ConfigPin(GPIOA, BSP_BUTTON_PIN, GPIO_MODE_INPUT_PULLUP, GPIO_SPEED_INPUT);
}

void bsp_reboot_to_bootloader(void)
{
    __asm volatile ("cpsid i" ::: "memory");
    bsp_relay_force_off();
    BSP_RCC_APB1ENR |= (1UL << 28) | (1UL << 27);       /* PWREN | BKPEN */
    (void)BSP_RCC_APB1ENR;
    BSP_PWR_CR |= (1UL << 8);                           /* DBP: cho phép ghi backup domain */
    BSP_BKP_DR1 = OTA_BKP_FLAG_UPDATE;
    __asm volatile ("dsb" ::: "memory");
    BSP_SCB_AIRCR = (0x05FAUL << 16) | (1UL << 2);      /* SYSRESETREQ */
    __asm volatile ("dsb" ::: "memory");
    for (;;) { }
}

void bsp_panic(void)
{
    __asm volatile ("cpsid i" ::: "memory");
    bsp_relay_force_off();
    for (;;) { }
}
