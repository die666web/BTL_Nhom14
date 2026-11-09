#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/* Leader owns this file. Reset clock = HSI 8 MHz, APB1/APB2 divider = 1.
 * If clock changes, update UART BRR, timer prescaler and FreeRTOS together. */
#define BSP_CPU_CLOCK_HZ       8000000UL
#define BSP_APB2_CLOCK_HZ      BSP_CPU_CLOCK_HZ
#define BSP_UART_BAUD          9600UL
#define BSP_I2C_SCL_PIN        6U  /* PB6 */
#define BSP_I2C_SDA_PIN        7U  /* PB7; external pull-ups to 3.3 V */
#define BSP_RELAY_PIN          8U  /* PB8 */
#define BSP_BUTTON_PIN         0U  /* PA0, button to GND */
#define BSP_BUTTON_ACTIVE      0U
/* ASSUMPTION: active-low relay. Confirm with module BEFORE connecting fan. */
#define BSP_RELAY_ACTIVE       0U
#define BSP_UART_TX_PIN        9U  /* PA9 */
#define BSP_UART_RX_PIN       10U  /* PA10 */
#define BSP_INA219_ADDR7       0x40U /* confirm A0/A1 */
#define BSP_OLED_ADDR7         0x3CU /* confirm controller/address on module */

#endif
