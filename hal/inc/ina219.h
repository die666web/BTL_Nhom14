#ifndef INA219_H
#define INA219_H
#include "hal_status.h"
/* API đã thống nhất. Người phụ trách: Lê Trọng Mạnh.
 * Dùng hal_i2c; kiểm tra shunt thực tế và calibration, đọc bus/shunt/current có dấu, báo lỗi overflow/timeout. Không copy driver I2C vào file này.
 * Thay đổi API: trao đổi leader trước khi sửa. */
hal_status_t ina219_init(float shunt_ohm, float max_current_a);
hal_status_t ina219_read(float *voltage_v, float *current_a);
#endif
