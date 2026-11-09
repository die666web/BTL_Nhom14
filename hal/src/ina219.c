/* CHƯA TRIỂN KHAI — Lê Trọng Mạnh
 * File dành riêng cho phần việc này, không phải module đã chạy.
 * Đọc docs/PHAN_CONG.md và header cùng tên trước khi viết.
 * Dùng hal_i2c; kiểm tra shunt thực tế và calibration, đọc bus/shunt/current có dấu, báo lỗi overflow/timeout. Không copy driver I2C vào file này.
 * Điền các hàm khai báo trong header; helper nội bộ để static.
 * Không sửa main.c, BSP, FreeRTOSConfig.h hoặc API chung trong nhánh này.
 * Không dùng delay_ms/HAL_Delay trong task, không chờ vô hạn.
 * Khi xong: test riêng trên kit, ghi kết quả vào tests/manual/, mở PR -> develop.
 */
#include "ina219.h"

/* TODO: viết phần triển khai ở đây. Không có stub giả trả về thành công. */
