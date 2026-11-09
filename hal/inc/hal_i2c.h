#ifndef HAL_I2C_H
#define HAL_I2C_H
#include <stddef.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "hal_status.h"
/* API đã thống nhất. Người phụ trách: Lê Trọng Mạnh.
 * I2C1 PB6/PB7, địa chỉ 7 bit. init trước scheduler. Mỗi transfer tự lấy/nhả g_i2c_mutex; timeout tổng gồm cả chờ mutex/bus. Mem read dùng repeated START; xử lý NACK/BUSY và nhả mutex mọi nhánh lỗi. OLED/INA219 KHÔNG lấy mutex lần nữa.
 * Thay đổi API: trao đổi leader trước khi sửa. */
hal_status_t hal_i2c_init(void);
hal_status_t hal_i2c_write(uint8_t addr7, const uint8_t *data, size_t length, TickType_t timeout);
hal_status_t hal_i2c_mem_read(uint8_t addr7, uint8_t reg, uint8_t *data, size_t length, TickType_t timeout);
#endif
