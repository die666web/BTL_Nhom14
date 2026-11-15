#ifndef DEBUG_CONSOLE_H
#define DEBUG_CONSOLE_H
/* Console debug qua USB Serial (115200) để test không cần Wi-Fi/trình duyệt.
 * Lệnh (kết thúc bằng xuống dòng):
 *   help            liệt kê lệnh
 *   ls              liệt kê file LittleFS
 *   status          firmware đang lưu + trạng thái uart_bridge
 *   put <size>      nhận <size> byte nhị phân vào fw_store (giống /upload).
 *                   ESP32 trả "PUT READY", PC gửi từng khối <= 256 B, mỗi khối chờ "." rồi gửi tiếp.
 *                   Kết thúc: "PUT OK <size> <crc>" hoặc "PUT ERR <lý do>".
 *   flash           nạp /fw.bin xuống STM32 (giống nút trên web)
 * Công cụ PC: tools/ota_test.py serial-put. */
void debug_console_loop(void);
#endif
