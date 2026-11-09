#ifndef RELAY_H
#define RELAY_H
#include <stdbool.h>
/* API đã thống nhất. Người phụ trách: Vũ Văn Võ.
 * PB8; lấy mức active từ board_config.h. relay_init và mọi lỗi/OTA đều OFF. Chỉ TaskControl gọi set/get. Không PWM relay; không tự đổi pin.
 * Thay đổi API: trao đổi leader trước khi sửa. */
void relay_init(void);
void relay_set(bool on);
bool relay_get(void);
#endif
