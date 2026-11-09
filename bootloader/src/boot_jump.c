/* CHƯA TRIỂN KHAI — Lê Trọng Mạnh
 * File dành riêng cho phần việc này, không phải module đã chạy.
 * Đọc docs/PHAN_CONG.md và header cùng tên trước khi viết.
 * Dừng IRQ/SysTick, đặt VTOR/MSP, nhảy reset handler đúng thứ tự. Đồng bộ clock handoff với BSP. Chỉ gọi sau validate.
 * Điền các hàm khai báo trong header; helper nội bộ để static.
 * Không sửa main.c, BSP, FreeRTOSConfig.h hoặc API chung trong nhánh này.
 * Không dùng delay_ms/HAL_Delay trong task, không chờ vô hạn.
 * Khi xong: test riêng trên kit, ghi kết quả vào tests/manual/, mở PR -> develop.
 */
#include "boot_jump.h"

/* TODO: viết phần triển khai ở đây. Không có stub giả trả về thành công. */
