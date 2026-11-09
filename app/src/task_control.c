/* CHƯA TRIỂN KHAI — Vũ Văn Võ
 * File dành riêng cho phần việc này, không phải module đã chạy.
 * Đọc docs/PHAN_CONG.md và header cùng tên trước khi viết.
 * Chu kỳ 20ms: đọc nút/chống dội, nhận g_command_queue, điều khiển relay. Ưu tiên OTA_REQUEST; khóa ON, tắt relay rồi set OFF_ACK. Publish g_control_queue.
 * Điền các hàm khai báo trong header; helper nội bộ để static.
 * Không sửa main.c, BSP, FreeRTOSConfig.h hoặc API chung trong nhánh này.
 * Không dùng delay_ms/HAL_Delay trong task, không chờ vô hạn.
 * Khi xong: test riêng trên kit, ghi kết quả vào tests/manual/, mở PR -> develop.
 */
#include "task_control.h"

/* TODO: viết phần triển khai ở đây. Không có stub giả trả về thành công. */
