#ifndef BOOT_PORT_H
#define BOOT_PORT_H
/* Lớp phần cứng tối thiểu của bootloader (thanh ghi trực tiếp, không FreeRTOS, không ngắt UART).
 * Bản target: bootloader/src/boot_port.c. Bản giả lập PC: tests/host/test_ota.c. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Đọc và XOÁ cờ update trong BKP_DR1. Gọi đầu tiên trong main. */
bool     port_take_update_flag(void);
/* SysTick 1 ms, USART1 115200 8N1 (PA9/PA10), LED PC13, giữ relay OFF. */
void     port_init(void);
uint32_t port_ms(void);
/* 1: có byte; 0: chưa có; -1: lỗi UART (ORE/FE/NE/PE), byte đã bị bỏ. */
int      port_uart_getc(uint8_t *byte);
/* Gửi chặn, trả về khi byte cuối đã ra khỏi dây (cờ TC). */
void     port_uart_write(const uint8_t *data, size_t len);
void     port_led(bool on);
/* Trả USART/GPIO/LED về trạng thái reset trước khi nhảy sang app (relay vẫn OFF). */
void     port_deinit(void);

#endif
