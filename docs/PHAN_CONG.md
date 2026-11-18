# Giao việc theo file — HTNN2-N14

Căn cứ đề cương HTNN2-N14(1) và quy trình Git của thầy. Phần cứng hiện tại:
INA219 + OLED chung I2C PB6/PB7, relay PA0 (kích mức cao), nút PB8, ESP32 qua USART1 PA9/PA10.
Không có yêu cầu giọng nói, PWM tốc độ hay LCD1602 trong bản đề cương này.

## Vũ Văn Võ — relay, nút nhấn, TaskControl

| File được viết | Viết gì | Hoàn thành khi |
|---|---|---|
| `hal/src/relay.c` | **Đã viết, có test PC.** `relay_init/set/get`, mức active từ BSP | Khởi động OFF; ON/OFF đúng trên kit |
| `hal/src/button.c` | **Đã viết, có test PC.** `button_init/poll_pressed`, chống dội 3 mẫu x 20 ms | Mỗi nhấn một sự kiện; giữ nút không lặp |
| `app/src/task_control.c` | **Đã viết.** Chu kỳ 20ms, nút nhấn, publish trạng thái, khóa relay khi OTA | Đọc nút; OTA ưu tiên tắt và khóa relay; chỉ ACK sau OFF |
| `tests/manual/VO.md` | Cách nối, bước thử, kết quả, ảnh/log | Có test giữ nút, dội, OTA đang ON |

Header tương ứng ở `hal/inc/` và `app/inc/` đã chốt; không tự đổi chữ ký hàm.
**Không viết lại GPIO, RCC, UART; không sửa main, startup, FreeRTOSConfig.**
Việc còn lại: test trên kit, ghi `tests/manual/VO.md`.

## Vũ Chí Công — OLED, TaskDisplay

| File được viết | Viết gì | Hoàn thành khi |
|---|---|---|
| `hal/src/oled.c` | Init controller thực tế, font/render và `oled_show`; gọi HAL I2C | Hiển thị được dummy data; lỗi bus trả về, không treo |
| `app/src/task_display.c` | Task static + `task_display_create`; peek hai mailbox, 400ms | U(V), I(mA), P(W), trạng thái, version; báo sensor invalid |
| `tests/manual/CONG.md` | Model/địa chỉ OLED, ảnh màn, test lỗi | Test số âm/0, mất cảm biến, dữ liệu cũ và trạng thái OFF/ON |

Không đọc INA219 hay GPIO relay trực tiếp. Không tự viết lại code I2C trong `oled.c`; chỉ gọi `hal_i2c`.
I2C chung đã có (`hal/inc/hal_i2c.h`): gửi lệnh bằng `hal_i2c_mem_write(BSP_OLED_ADDR7, 0x00, cmd, n, t)`,
gửi dữ liệu màn bằng `hal_i2c_mem_write(BSP_OLED_ADDR7, 0x40, buf, n, t)`; dò màn bằng `hal_i2c_is_ready`.
Timeout ~0.1 ms/byte; cả khung 1 KiB cần >= 150 ms. Không merge mock vào firmware thật.
Nếu cần font riêng: thêm `hal/inc/oled_font.h`/`hal/src/oled_font.c`, nhờ leader thêm vào Makefile + Keil.
Nhánh: `feature/oled`, sau đó `feature/task-display`.

## Lê Trọng Mạnh — nền chung, INA219, truyền thông, OTA, tích hợp

| File | Việc còn phải viết |
|---|---|
| `hal/src/hal_i2c.c` | **Đã viết, chờ test kit.** I2C1 100 kHz polling; mutex trong từng transaction, timeout, NACK, recovery 9 xung SCL |
| `hal/src/ina219.c` | **Đã viết.** Calibration theo shunt; đọc U/I, báo lỗi/overflow |
| `app/src/task_measure.c` | **Đã viết.** Chu kỳ 200ms, lọc, P=U*I, publish valid/timestamp |
| `app/src/task_comm.c` | Dò lệnh `#ENTER_BOOTLOADER#`, OTA REQUEST/OFF_ACK |
| `bsp/bsp.c` | Cờ BKP, nhảy vào bootloader ROM của ST, giữ relay OFF |
| `esp32/src/main.cpp` | Entry ESP32, Wi-Fi/web setup |
| `esp32/src/web_ota.cpp` + `esp32/data/index.html` | Upload, tiến độ/lỗi/phiên bản |
| `esp32/src/uart_bridge.cpp` | Giao thức AN3155: xoá, ghi, đọc lại, Go |
| `tests/manual/MANH.md` | Đo, I2C tranh chấp, UART/OTA, heap/stack |

Nền **đã chuẩn bị, chỉ kiểm tra/tích hợp, không giao viết lại**:
`bsp/*`, `app_ipc.*`, `app_cfg.h`, `app_types.h`, `rtos_hooks.c`, `main.c`,
`startup.c`, `FreeRTOSConfig.h`, Makefile, project Keil, GPIO/RCC/UART.
Bốn task phải do người nhận nhiệm vụ viết thật; bản khung chưa tạo task giả.

## Trình tự giao việc

1. Leader kiểm tra board/polarity, build cả môi trường và chạy khung trên kit.
2. Mạnh làm I2C trước; Võ có thể làm relay/button ngay; Công chốt model OLED và chuẩn bị UI/mock.
3. Code thiết bị trong `hal/` (relay/button/OLED/INA219) đã qua PR rồi mới viết task riêng, không chạm main.
4. Mạnh tích hợp trên `integration/system-v1`, bật từng `APP_ENABLE_*`, đo heap/stack và test.
5. Chức năng đo/điều khiển/hiển thị ổn mới ghép OTA. Dùng bootloader ROM của ST, không tự viết bootloader.
6. Chỉ release vào main sau test phần cứng. Không gắn nhãn “ổn định” chỉ vì build được.
