/* CHƯA TRIỂN KHAI — Lê Trọng Mạnh
 * File dành riêng cho phần việc này, không phải module đã chạy.
 * Đọc docs/PHAN_CONG.md và header cùng tên trước khi viết.
 * Nhận byte UART có timeout, parse gói; gửi lệnh qua g_command_queue. OTA: clear ACK cũ, set REQUEST, chờ OFF_ACK hữu hạn trước reset. Không tự điều khiển relay.
 * Điền các hàm khai báo trong header; helper nội bộ để static.
 * Không sửa main.c, BSP, FreeRTOSConfig.h hoặc API chung trong nhánh này.
 * Không dùng delay_ms/HAL_Delay trong task, không chờ vô hạn.
 * Khi xong: test riêng trên kit, ghi kết quả vào tests/manual/, mở PR -> develop.
 */
#include "task_comm.h"

/* TODO: viết phần triển khai ở đây. Không có stub giả trả về thành công. */
