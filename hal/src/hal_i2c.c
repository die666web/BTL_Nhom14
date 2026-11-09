/* CHƯA TRIỂN KHAI — Lê Trọng Mạnh
 * File dành riêng cho phần việc này, không phải module đã chạy.
 * Đọc docs/PHAN_CONG.md và header cùng tên trước khi viết.
 * I2C1 PB6/PB7, địa chỉ 7 bit. init trước scheduler. Mỗi transfer tự lấy/nhả g_i2c_mutex; timeout tổng gồm cả chờ mutex/bus. Mem read dùng repeated START; xử lý NACK/BUSY và nhả mutex mọi nhánh lỗi. OLED/INA219 KHÔNG lấy mutex lần nữa.
 * Điền các hàm khai báo trong header; helper nội bộ để static.
 * Không sửa main.c, BSP, FreeRTOSConfig.h hoặc API chung trong nhánh này.
 * Không dùng delay_ms/HAL_Delay trong task, không chờ vô hạn.
 * Khi xong: test riêng trên kit, ghi kết quả vào tests/manual/, mở PR -> develop.
 */
#include "hal_i2c.h"

/* TODO: viết phần triển khai ở đây. Không có stub giả trả về thành công. */
