#ifndef UART_BRIDGE_H
#define UART_BRIDGE_H
/* Gửi firmware từ LittleFS xuống bootloader STM32 theo OTA UART v1 (docs/OTA_CONTRACT.md mục 6).
 * Máy trạng thái không chặn: gọi uart_bridge_loop() liên tục trong loop(). */
void        uart_bridge_begin(void);              /* mở UART nối STM32 */
bool        uart_bridge_start(const char *path);  /* false nếu đang bận hoặc file sai */
bool        uart_bridge_busy(void);
const char *uart_bridge_state_text(void);         /* chuỗi trạng thái cho web */
void        uart_bridge_loop(void);
#endif
