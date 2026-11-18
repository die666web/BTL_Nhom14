#include "hal_uart.h"
#include "hal_gpio.h"
#include "hal_rcc.h"
#include "board_config.h"

static volatile uint8_t rx_buffer[UART_BUFFER_SIZE];
static volatile uint8_t rx_head = 0;
static uint8_t line_index = 0;
static volatile uint8_t rx_tail = 0;

static volatile uint8_t tx_buffer[UART_BUFFER_SIZE];
static volatile uint8_t tx_head = 0;
static volatile uint8_t tx_tail = 0;

static uint8_t UART_NextIndex(uint8_t index)
{
    index++;

    if (index >= UART_BUFFER_SIZE)
    {
        index = 0;
    }

    return index;
}

static uint8_t SendInterrupt_UART(uint8_t data)
{
    uint8_t next = UART_NextIndex(tx_head);

    if (next == tx_tail)
    {
        return 0;
    }

    tx_buffer[tx_head] = data;
    tx_head = next;

    /* TXEIE: cho phep ngat khi thanh ghi phat trong. */
    USART1->CR1 |= (1U << 7);

    return 1;
}

void Config_uart(void)
{
    RCC_EnableGPIOAClock();
    RCC_EnableUSART1Clock();

    GPIO_ConfigPin(GPIOA, 9, GPIO_MODE_AF_PP, GPIO_SPEED_50MHZ);
    GPIO_ConfigPin(GPIOA, 10, GPIO_MODE_INPUT_FLOATING, GPIO_SPEED_INPUT);

    /* 8 MHz / 115200 = 69 (ESP32 va bootloader cung dung 115200). */
    USART1->BRR = (BSP_APB2_CLOCK_HZ + BSP_UART_BAUD / 2U) / BSP_UART_BAUD;
    USART1->CR1 |= (1U << 2) | (1U << 3) | (1U << 5) | (1U << 13);
    NVIC_ISER1 = (1U << 5);    /* Enable USART1 IRQ 37 */
}

void SendStringInterrupt_UART(const char *str)
{
    while (*str != '\0')
    {
        while (!SendInterrupt_UART((uint8_t)*str))
        {
        }

        str++;
    }

    while (!SendInterrupt_UART('\n'))
    {
    }
}

uint8_t ReceiveInterrupt_UART(uint8_t *out_data)
{
    if (rx_tail == rx_head)
    {
        return 0;
    }

    *out_data = rx_buffer[rx_tail];
    rx_tail = UART_NextIndex(rx_tail);

    return 1;
}

void USART1_IRQHandler(void)
{
    /* TXE va TXEIE cung bang 1: gui byte tiep theo. */
    if ((USART1->SR & (1U << 7)) &&
        (USART1->CR1 & (1U << 7)))
    {
        if (tx_tail != tx_head)
        {
            USART1->DR = tx_buffer[tx_tail];
            tx_tail = UART_NextIndex(tx_tail);
        }

        if (tx_tail == tx_head)
        {
            USART1->CR1 &= ~(1U << 7);
        }
    }

    /* RXNE va RXNEIE cung bang 1: luu byte vao RX buffer. */
    if ((USART1->SR & (1U << 5)) &&
        (USART1->CR1 & (1U << 5)))
    {
        uint8_t receive = (uint8_t)USART1->DR;
        uint8_t next = UART_NextIndex(rx_head);

        if (next != rx_tail)
        {
            rx_buffer[rx_head] = receive;
            rx_head = next;
        }
    }
}

uint8_t UART_ReadLine(char *buffer, uint8_t max_size)
{
    uint8_t data;

    if (!ReceiveInterrupt_UART(&data))
    {
        return 0;
    }

    if (data == '\r' || data == '\n')
    {
        if (line_index == 0)
        {
            return 0;
        }

        buffer[line_index] = '\0';
        line_index = 0;

        return 1;
    }

    if (line_index < max_size - 1)
    {
        buffer[line_index++] = (char)data;
    }
    else
    {
        // Buffer đầy thì nhận lại từ đầu
        line_index = 0;
        buffer[0] = '\0';
    }

    return 0;
}

uint8_t StringEqual(const char *s1, const char *s2)
{
    while (*s1 != '\0' && *s2 != '\0')
    {
        if (*s1 != *s2)
        {
            return 0;
        }

        s1++;
        s2++;
    }

    return (*s1 == '\0' && *s2 == '\0');
}

void SendNumberInterrupt_UART(uint8_t number)
{
    if (number >= 100)
    {
        while (!SendInterrupt_UART(
            (uint8_t)('0' + number / 100)
        ));

        while (!SendInterrupt_UART(
            (uint8_t)('0' + (number / 10) % 10)
        ));

        while (!SendInterrupt_UART(
            (uint8_t)('0' + number % 10)
        ));
    }
    else if (number >= 10)
    {
        while (!SendInterrupt_UART(
            (uint8_t)('0' + number / 10)
        ));

        while (!SendInterrupt_UART(
            (uint8_t)('0' + number % 10)
        ));
    }
    else
    {
        while (!SendInterrupt_UART(
            (uint8_t)('0' + number)
        ));
    }
}

void SendTextInterrupt_UART(const char *text)
{
    while (*text != '\0')
    {
        while (!SendInterrupt_UART((uint8_t)*text))
        {
        }

        text++;
    }
}