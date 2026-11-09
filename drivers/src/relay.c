/* CHƯA TRIỂN KHAI — Vũ Văn Võ
 * File dành riêng cho phần việc này, không phải module đã chạy.
 * Đọc docs/PHAN_CONG.md và header cùng tên trước khi viết.
 * PB8; lấy mức active từ board_config.h. relay_init và mọi lỗi/OTA đều OFF. Chỉ TaskControl gọi set/get. Không PWM relay; không tự đổi pin.
 * Điền các hàm khai báo trong header; helper nội bộ để static.
 * Không sửa main.c, BSP, FreeRTOSConfig.h hoặc API chung trong nhánh này.
 * Không dùng delay_ms/HAL_Delay trong task, không chờ vô hạn.
 * Khi xong: test riêng trên kit, ghi kết quả vào tests/manual/, mở PR -> develop.
 */
#include "relay.h"

/* TODO: viết phần triển khai ở đây. Không có stub giả trả về thành công. */
