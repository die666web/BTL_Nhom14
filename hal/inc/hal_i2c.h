#ifndef HAL_I2C_H
#define HAL_I2C_H
#include <stddef.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "hal_status.h"
/* Thư viện I2C1 dùng chung cho mọi thiết bị trên bus (INA219, OLED, ...). Người phụ trách: Lê Trọng Mạnh.
 * PB6 SCL / PB7 SDA, 100 kHz, địa chỉ 7 bit (KHÔNG dịch trái, ví dụ OLED 0x3C).
 * hal_i2c_init() một lần trước scheduler, sau app_ipc_init(). Các hàm còn lại: chỉ gọi từ task.
 * Mỗi hàm là MỘT transaction trọn vẹn, tự lấy/nhả g_i2c_mutex: driver KHÔNG lấy mutex lần nữa.
 * timeout = tổng thời gian (chờ mutex + truyền), theo tick, không nhận portMAX_DELAY.
 *   Ước lượng: ~0.1 ms/byte. Ví dụ 1 khung OLED 128x64 (1025 byte) ~ 100 ms -> truyền >= 150 ms.
 * Trả về: HAL_OK; HAL_ERROR = thiết bị NACK hoặc lỗi bus (đã tự recovery);
 *         HAL_TIMEOUT = hết thời gian (bus đã được reset); HAL_INVALID_ARGUMENT; HAL_NOT_READY = chưa init.
 * Thay đổi API: trao đổi leader trước khi sửa. */
hal_status_t hal_i2c_init(void);

/* Kiểm tra thiết bị có trả ACK ở địa chỉ addr7 không (dò thiết bị, kiểm tra đấu dây). */
hal_status_t hal_i2c_is_ready(uint8_t addr7, TickType_t timeout);

/* START | addr+W | data[0..length-1] | STOP */
hal_status_t hal_i2c_write(uint8_t addr7, const uint8_t *data, size_t length, TickType_t timeout);

/* START | addr+R | data[0..length-1] | STOP */
hal_status_t hal_i2c_read(uint8_t addr7, uint8_t *data, size_t length, TickType_t timeout);

/* START | addr+W | reg | data[0..length-1] | STOP
 * INA219: reg = thanh ghi, data = 2 byte MSB trước.
 * OLED SSD1306/SH1106: reg = byte điều khiển 0x00 (lệnh) hoặc 0x40 (dữ liệu), data = buffer, không cần copy. */
hal_status_t hal_i2c_mem_write(uint8_t addr7, uint8_t reg, const uint8_t *data, size_t length, TickType_t timeout);

/* START | addr+W | reg | repeated START | addr+R | data[0..length-1] | STOP */
hal_status_t hal_i2c_mem_read(uint8_t addr7, uint8_t reg, uint8_t *data, size_t length, TickType_t timeout);
#endif
