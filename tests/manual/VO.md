# Nhật ký kiểm thử — Võ (relay + nút nhấn)

Code: `hal/src/relay.c`, `hal/src/button.c`, `app/src/task_control.c`.

## Đấu nối

| Module | Blue Pill |
|---|---|
| Relay IN | PA0 (kích mức cao) |
| Relay VCC / GND | 5 V / GND |
| Nút nhấn | PB8 ↔ GND |

## Các bước thử

Nạp `firmware.bin` bình thường (TaskControl đã bật).

| # | Thao tác | Kỳ vọng | Kết quả |
|---|---|---|---|
| 1 | Cấp nguồn | Relay tắt, không "tách" lúc khởi động | ☐ |
| 2 | Nhấn nút một lần | Relay bật | ☐ |
| 3 | Nhấn lần nữa | Relay tắt | ☐ |
| 4 | Giữ nút 3 s | Chỉ đổi trạng thái một lần | ☐ |
| 5 | Giữ nút khi cấp nguồn | Relay vẫn tắt đến khi thả rồi nhấn lại | ☐ |

- Ngày / commit / người chạy:
- Ảnh / ghi chú:
