#ifndef UART_H
#define UART_H

#include <stdint.h>

#define RCC_BASE        0x40021000UL
#define GPIOA_BASE      0x40010800UL
#define USART1_BASE     0x40013800UL

#define NVIC_ISER1      (*(volatile uint32_t *)0xE000E104UL)
#define UART_BUFFER_SIZE 128U

typedef struct
{
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

#define USART1  ((USART_TypeDef *)USART1_BASE)

/* Khoi tao USART1: PA9 TX, PA10 RX, baud = BSP_UART_BAUD (115200), PCLK2 = 8 MHz. */
void Config_uart(void);

/* Gui mot chuoi bang TX interrupt. Ham tu them ky tu xuong dong. */
void SendStringInterrupt_UART(const char *str);

/*
 * Lay mot byte tu RX buffer.
 * Tra ve 1 neu co du lieu, tra ve 0 neu buffer rong.
 */
uint8_t ReceiveInterrupt_UART(uint8_t *out_data);

/* Ten nay phai trung voi vector table trong startup.c. */
void USART1_IRQHandler(void);

uint8_t StringEqual(const char *s1, const char *s2);
void SendNumberInterrupt_UART(uint8_t number);
uint8_t UART_ReadLine(char *buffer, uint8_t max_size);
void SendTextInterrupt_UART(const char *text);

#endif
