#ifndef OLED_H
#define OLED_H
#include "hal_status.h"
#include "app_types.h"
/* API đã thống nhất. Người phụ trách: Vũ Chí Công.
 * Xác nhận IC/kích thước màn thực tế trước code (đề cương chưa chốt SSD1306). Dùng hal_i2c_write và timeout; font/UI nằm đây. Chỉ TaskDisplay sử dụng, không khóa g_i2c_mutex bên ngoài HAL.
 * Thay đổi API: trao đổi leader trước khi sửa. */
hal_status_t oled_init(void);
hal_status_t oled_show(const app_measurement_t *measurement,
                       const app_control_state_t *control, const char *version);
#endif
