#ifndef HAL_UART_H
#define HAL_UART_H
#include <stddef.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "hal_status.h"
/* Init once before scheduler. TX mutex per message, finite timeout, task only.
 * RX: one owner TaskComm; do not mix raw RX and ReadLine. ISR uses FromISR API.
 * PA9/PA10, baud from board_config.h. Never log text on OTA data UART. */
BaseType_t Config_uart(void);
hal_status_t hal_uart_write(const uint8_t *data, size_t length, TickType_t timeout);
hal_status_t hal_uart_read(uint8_t *out, TickType_t timeout);
uint32_t hal_uart_rx_error_count(void);
/* Existing helpers now return status; check it. Whole-call deadline 1000ms.
 * HAL_OK means queued, not wire idle. Timeout may have transmitted a prefix. */
hal_status_t SendStringInterrupt_UART(const char *text);
hal_status_t SendTextInterrupt_UART(const char *text);
hal_status_t SendNumberInterrupt_UART(uint8_t number);
uint8_t ReceiveInterrupt_UART(uint8_t *out);
uint8_t UART_ReadLine(char *buffer, uint8_t max_size);
uint8_t StringEqual(const char *a, const char *b);
void USART1_IRQHandler(void);
#endif
