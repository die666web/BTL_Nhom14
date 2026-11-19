# Demo nạp firmware qua ESP32 (OTA)

| File | Chạy gì | Nhìn trên kit |
|---|---|---|
| `1_app_chinh.bin` | App thật: đo INA219, OLED, relay + nút | LED PC13 **tắt**, OLED hiển thị |
| `2_led_pc13_nhay_nhanh.bin` | App thử: chỉ nháy LED PC13 | LED PC13 **nháy nhanh** (5 lần/giây), OLED đứng |

Cả hai bản đều nghe lệnh cập nhật từ ESP32, nên nạp qua lại bao nhiêu lần cũng được, không cần ST-Link.

## Các bước demo

1. Kit đang chạy `1_app_chinh.bin` (LED PC13 tắt).
2. Điện thoại/laptop vào Wi-Fi của ESP32, mở `http://192.168.4.1`.
3. Chọn `2_led_pc13_nhay_nhanh.bin`, bấm upload, rồi bấm nạp. Khoảng 6 giây sau LED PC13 nháy nhanh: firmware mới đã được ghi vào Flash và chạy.
4. Upload `1_app_chinh.bin` và nạp lại để trở về app thật: LED tắt, OLED chạy lại.

## Hệ thống nạp thế nào

ESP32 gửi chuỗi `#ENTER_BOOTLOADER#` cho app STM32 -> app tắt relay, reset vào bootloader ROM có sẵn của ST (0x1FFFF000)
-> ESP32 nói chuyện với bootloader theo giao thức AN3155: xoá Flash, ghi từng khối 256 B và đọc lại so sánh, rồi lệnh Go chạy app mới.

Tạo lại các file: `make all ota-led` (ra `build/gcc/firmware.bin` và `build/gcc/ota_led_test.bin`).
