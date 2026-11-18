# Kiểm thử — đã làm gì, chưa làm gì

## Test PC (`sh tests/run_tests.sh`, Linux + gcc)

Chạy logic driver trên máy tính: thanh ghi ngoại vi được giả bằng RAM (mmap), FreeRTOS queue/mutex
giả đơn giản. Không mô phỏng thời gian điện hay ngắt thật.

| Module | Test | Kiểm tra |
|---|---|---|
| UART | `test_uart_init` | BRR đúng baud, không init hai lần |
| | `test_uart_send_number` | In đúng 0, 101, 255 |
| | `test_uart_read_line` | Nhận một dòng CRLF |
| | `test_uart_bad_args` | Từ chối NULL và chờ vô hạn |
| GPIO | `test_gpio_config_output` | Ghi đúng nibble CRH |
| | `test_gpio_reject_bad_speed` | Output với tốc độ sai không đổi thanh ghi |
| | `test_gpio_write` | Ghi 1/0 qua BSRR/BRR |
| Relay | `test_relay_init_off` | Khởi động ở OFF, output push-pull |
| | `test_relay_on` / `test_relay_off` | Mức chân và `relay_get` đúng |
| Nút nhấn | `test_button_press_once` | Một lần nhấn = một sự kiện |
| | `test_button_hold_no_repeat` | Giữ nút không lặp, thả không báo |
| | `test_button_bounce_filtered` | Lọc dội |
| | `test_button_held_at_boot` | Giữ nút lúc cấp nguồn không tính |

## Trên phần cứng

| Hạng mục | Kết quả |
|---|---|
| ESP32: upload file qua USB/web, gửi 56 320 B tới STM32 nhận ACK(END) | Đã thử |
| Mức kích relay (PA0 = 1 -> đóng) | Đã xác nhận với module |
| Relay/nút/TaskControl chạy cùng hệ thống | Chưa ghi kết quả — `tests/manual/VO.md` |
| OLED, INA219, I2C | Chưa — ghi vào `tests/manual/CONG.md`, `MANH.md` |
| Build Keil ARM Compiler 6 | Chưa chạy Rebuild All |
| OTA trọn vòng trên Blue Pill (app → bootloader → app mới) | Chưa |

Hạng mục "Chưa" không được coi là đã đạt.
