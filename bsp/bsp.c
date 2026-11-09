#include "bsp.h"
#include "board_config.h"
#include "hal_gpio.h"
#include "hal_rcc.h"

void bsp_relay_force_off(void)
{
    GPIO_WritePin(GPIOB, BSP_RELAY_PIN, !BSP_RELAY_ACTIVE);
}

void bsp_system_init(void)
{
    /* Standalone application starts after reset, with HSI at 8 MHz.
     * Bootloader hand-off clock/VTOR must be designed before OTA integration. */
    RCC_EnableGPIOAClock();
    RCC_EnableGPIOBClock();
    /* Preload inactive level before switching PB8 to output. */
    bsp_relay_force_off();
    GPIO_ConfigPin(GPIOB, BSP_RELAY_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_2MHZ);
    GPIO_ConfigPin(GPIOA, BSP_BUTTON_PIN, GPIO_MODE_INPUT_PULLUP, GPIO_SPEED_INPUT);
}

void bsp_panic(void)
{
    __asm volatile ("cpsid i" ::: "memory");
    bsp_relay_force_off();
    for (;;) { }
}
