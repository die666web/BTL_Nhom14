/* CHƯA TRIỂN KHAI — Vũ Chí Công
 * File dành riêng cho phần việc này, không phải module đã chạy.
 * Đọc docs/PHAN_CONG.md và header cùng tên trước khi viết.
 * Xác nhận IC/kích thước màn thực tế trước code (đề cương chưa chốt SSD1306). Dùng hal_i2c_write và timeout; font/UI nằm đây. Chỉ TaskDisplay sử dụng, không khóa g_i2c_mutex bên ngoài HAL.
 * Điền các hàm khai báo trong header; helper nội bộ để static.
 * Không sửa main.c, BSP, FreeRTOSConfig.h hoặc API chung trong nhánh này.
 * Không dùng delay_ms/HAL_Delay trong task, không chờ vô hạn.
 * Khi xong: test riêng trên kit, ghi kết quả vào tests/manual/, mở PR -> develop.
 */
#include "oled.h"

/* TODO: viết phần triển khai ở đây. Không có stub giả trả về thành công. */
