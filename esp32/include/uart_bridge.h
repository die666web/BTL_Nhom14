#ifndef UART_BRIDGE_H
#define UART_BRIDGE_H
/* Nạp firmware từ LittleFS xuống STM32 qua bootloader ROM của ST (giao thức AN3155).
 * uart_bridge_start() tạo một task FreeRTOS chạy việc nạp; web đọc tiến độ qua uart_bridge_state_text(). */
void        uart_bridge_begin(void);              /* mở UART nối STM32 */
bool        uart_bridge_start(const char *path);  /* false nếu đang bận */
bool        uart_bridge_busy(void);
const char *uart_bridge_state_text(void);         /* chuỗi trạng thái cho web */
#endif
