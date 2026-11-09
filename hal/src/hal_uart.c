#include "hal_uart.h"
#include "hal_gpio.h"
#include "hal_rcc.h"
#include "board_config.h"
#include "app_cfg.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include <string.h>

typedef struct {
    volatile uint32_t SR, DR, BRR, CR1, CR2, CR3, GTPR;
} uart_regs_t;
#define USART1 ((uart_regs_t *)0x40013800UL)
#define UART_LINE_SIZE 128U
static QueueHandle_t rx_queue, tx_queue;
static SemaphoreHandle_t tx_mutex;
static volatile uint32_t rx_errors;
static char line[UART_LINE_SIZE];
static size_t line_used;
static uint8_t line_discard;

BaseType_t Config_uart(void)
{
    if (rx_queue != NULL) return pdFAIL;
    rx_queue = xQueueCreate(APP_UART_RX_QUEUE_LEN, sizeof(uint8_t));
    tx_queue = xQueueCreate(128U, sizeof(uint8_t));
    tx_mutex = xSemaphoreCreateMutex();
    if (!rx_queue || !tx_queue || !tx_mutex) {
        if (rx_queue) vQueueDelete(rx_queue);
        if (tx_queue) vQueueDelete(tx_queue);
        if (tx_mutex) vSemaphoreDelete(tx_mutex);
        rx_queue = tx_queue = NULL; tx_mutex = NULL;
        return pdFAIL;
    }
    RCC_EnableGPIOAClock();
    RCC_EnableUSART1Clock();
    GPIO_ConfigPin(GPIOA, BSP_UART_TX_PIN, GPIO_MODE_AF_PP, GPIO_SPEED_50MHZ);
    GPIO_ConfigPin(GPIOA, BSP_UART_RX_PIN, GPIO_MODE_INPUT_FLOATING, GPIO_SPEED_INPUT);
    USART1->CR1 = 0U;
    USART1->CR2 = 0U;
    USART1->CR3 = 0U; /* interrupt TX, not DMA */
    USART1->BRR = (BSP_APB2_CLOCK_HZ + BSP_UART_BAUD / 2U) / BSP_UART_BAUD;
    (void)USART1->SR; (void)USART1->DR;
    /* IRQ37 priority 6 permits FromISR (max syscall priority = 5). */
    ((volatile uint8_t *)0xE000E400UL)[37] = (6U << 4);
    (*(volatile uint32_t *)0xE000E284UL) = (1UL << 5);
    (*(volatile uint32_t *)0xE000E104UL) = (1UL << 5);
    USART1->CR1 = (1U << 2) | (1U << 3) | (1U << 5) | (1U << 13);
    return pdPASS;
}

static TickType_t remaining(TickType_t start, TickType_t budget)
{
    TickType_t elapsed = xTaskGetTickCount() - start;
    return elapsed >= budget ? 0 : budget - elapsed;
}

static hal_status_t write_locked(const uint8_t *data, size_t length,
                                  TickType_t start, TickType_t timeout)
{
    for (size_t i = 0; i < length; ++i) {
        if (xQueueSend(tx_queue, &data[i], remaining(start, timeout)) != pdPASS)
            return HAL_TIMEOUT;
        taskENTER_CRITICAL();
        USART1->CR1 |= (1U << 7);
        taskEXIT_CRITICAL();
    }
    return HAL_OK;
}

static hal_status_t write_message(const uint8_t *data, size_t length,
                                  TickType_t timeout, uint8_t newline)
{
    if ((!data && length) || timeout == portMAX_DELAY) return HAL_INVALID_ARGUMENT;
    if (!tx_mutex || xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) return HAL_NOT_READY;
    TickType_t start = xTaskGetTickCount();
    if (xSemaphoreTake(tx_mutex, timeout) != pdPASS) return HAL_TIMEOUT;
    hal_status_t status = write_locked(data, length, start, timeout);
    if (status == HAL_OK && newline) {
        const uint8_t lf = '\n';
        status = write_locked(&lf, 1, start, timeout);
    }
    (void)xSemaphoreGive(tx_mutex);
    return status;
}

hal_status_t hal_uart_write(const uint8_t *data, size_t length, TickType_t timeout)
{ return write_message(data, length, timeout, 0); }

hal_status_t hal_uart_read(uint8_t *out, TickType_t timeout)
{
    if (!out || timeout == portMAX_DELAY) return HAL_INVALID_ARGUMENT;
    if (!rx_queue || xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) return HAL_NOT_READY;
    return xQueueReceive(rx_queue, out, timeout) == pdPASS ? HAL_OK : HAL_TIMEOUT;
}

void USART1_IRQHandler(void)
{
    BaseType_t wake = pdFALSE;
    uint32_t status = USART1->SR;
    if (status & ((1U << 5) | 0x0FU)) {
        /* Read SR then DR clears RXNE and ORE/FE/NE/PE. Drop corrupt byte. */
        uint8_t byte = (uint8_t)USART1->DR;
        if (status & 0x0FU) ++rx_errors;
        else if ((status & (1U << 5)) &&
                 xQueueSendFromISR(rx_queue, &byte, &wake) != pdPASS) ++rx_errors;
    }
    if ((status & (1U << 7)) && (USART1->CR1 & (1U << 7))) {
        uint8_t byte;
        if (xQueueReceiveFromISR(tx_queue, &byte, &wake) == pdPASS) USART1->DR = byte;
        else USART1->CR1 &= ~(1U << 7);
    }
    portYIELD_FROM_ISR(wake);
}

uint32_t hal_uart_rx_error_count(void) { return rx_errors; }
uint8_t ReceiveInterrupt_UART(uint8_t *out) { return hal_uart_read(out, 0) == HAL_OK; }
hal_status_t SendTextInterrupt_UART(const char *text)
{
    if (!text) return HAL_INVALID_ARGUMENT;
    return write_message((const uint8_t *)text, strlen(text), pdMS_TO_TICKS(1000), 0);
}
hal_status_t SendStringInterrupt_UART(const char *text)
{
    if (!text) return HAL_INVALID_ARGUMENT;
    return write_message((const uint8_t *)text, strlen(text), pdMS_TO_TICKS(1000), 1);
}
hal_status_t SendNumberInterrupt_UART(uint8_t number)
{
    uint8_t digits[3]; size_t n = 0;
    if (number >= 100U) digits[n++] = (uint8_t)('0' + number / 100U);
    if (number >= 10U) digits[n++] = (uint8_t)('0' + (number / 10U) % 10U);
    digits[n++] = (uint8_t)('0' + number % 10U);
    return hal_uart_write(digits, n, pdMS_TO_TICKS(1000));
}

uint8_t UART_ReadLine(char *buffer, uint8_t max_size)
{
    uint8_t byte;
    if (!buffer || max_size < 2U) return 0;
    while (ReceiveInterrupt_UART(&byte)) {
        if (byte == '\r' || byte == '\n') {
            uint8_t ready = !line_discard && line_used > 0 && line_used < max_size;
            if (ready) { memcpy(buffer, line, line_used); buffer[line_used] = '\0'; }
            line_used = 0; line_discard = 0;
            if (ready) return 1;
        } else if (!line_discard) {
            if (line_used < sizeof(line) - 1U) line[line_used++] = (char)byte;
            else line_discard = 1; /* discard entire overlong line */
        }
    }
    return 0;
}

uint8_t StringEqual(const char *a, const char *b)
{ return a && b && strcmp(a, b) == 0; }
