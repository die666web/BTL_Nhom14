#ifndef BUTTON_H
#define BUTTON_H
#include <stdbool.h>
/* API đã thống nhất. Người phụ trách: Vũ Văn Võ.
 * PA0 pull-up, nhấn nối GND. Gọi button_poll_pressed mỗi 20ms; true đúng một lần cho mỗi lần nhấn sau chống dội. Không delay bận trong driver.
 * Thay đổi API: trao đổi leader trước khi sửa. */
void button_init(void);
bool button_poll_pressed(void);
#endif
