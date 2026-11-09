# Giao việc theo file — HTNN2-N14

Căn cứ đề cương HTNN2-N14(1) và quy trình Git của thầy. Phần cứng hiện tại:
INA219 + OLED chung I2C PB6/PB7, relay PB8, nút PA0, ESP32 qua USART1 PA9/PA10.
Không có yêu cầu giọng nói, PWM tốc độ hay LCD1602 trong bản đề cương này.

## Vũ Văn Võ — relay, nút nhấn, TaskControl

| File được viết | Viết gì | Hoàn thành khi |
|---|---|---|
| `hal/src/relay.c` | `relay_init/set/get`, dùng GPIO đã có, mức active từ BSP | Khởi động OFF; ON/OFF đúng trên kit; không PWM |
| `hal/src/button.c` | `button_init/poll_pressed`, chống dội không delay | Mỗi nhấn một sự kiện; giữ nút không lặp ngoài ý muốn |
| `app/src/task_control.c` | Hàm task nội bộ static, `task_control_create`; chu kỳ 20ms, queue lệnh, cập nhật trạng thái | Đọc nút/ON/OFF; OTA ưu tiên tắt và khóa relay; chỉ ACK sau OFF |
| `tests/manual/VO.md` | Cách nối, bước thử, kết quả, ảnh/log | Có test giữ nút, dội, lệnh queue, OTA đang ON |

Header tương ứng ở `hal/inc/` và `app/inc/` đã chốt; không tự đổi chữ ký hàm.
**Không viết lại GPIO, RCC, UART; không sửa main, startup, FreeRTOSConfig.**
Nhánh gợi ý: `feature/relay-button`, sau đó `feature/task-control`.

## Vũ Chí Công — OLED, TaskDisplay

| File được viết | Viết gì | Hoàn thành khi |
|---|---|---|
| `hal/src/oled.c` | Init controller thực tế, font/render và `oled_show`; gọi HAL I2C | Hiển thị được dummy data; lỗi bus trả về, không treo |
| `app/src/task_display.c` | Task static + `task_display_create`; peek hai mailbox, 400ms | U(V), I(mA), P(W), trạng thái, version; báo sensor invalid |
| `tests/manual/CONG.md` | Model/địa chỉ OLED, ảnh màn, test lỗi | Test số âm/0, mất cảm biến, dữ liệu cũ và trạng thái OFF/ON |

Không đọc INA219 hay GPIO relay trực tiếp. Không tự viết lại code I2C trong `oled.c`; chỉ gọi `hal_i2c`.
Đợi I2C chung hoặc dùng mock dành cho test riêng; không merge mock vào firmware thật.
Nếu cần font riêng: thêm `hal/inc/oled_font.h`/`hal/src/oled_font.c`, nhờ leader cập nhật manifest.
**MAX7219 cũ không phải code OLED.** Chưa chốt IC OLED, không mặc định SSD1306 như sự thật.
Nhánh: `feature/oled`, sau đó `feature/task-display`.

## Lê Trọng Mạnh — nền chung, INA219, truyền thông, OTA, tích hợp

| File | Việc còn phải viết |
|---|---|
| `hal/src/hal_i2c.c` | I2C1 đúng API đã chốt; mutex nằm trong từng transaction, timeout và recovery |
| `hal/src/ina219.c` | Calibration theo shunt; đọc U/I, báo lỗi/overflow |
| `app/src/task_measure.c` | Chu kỳ 200ms, lọc, P=U*I, publish valid/timestamp |
| `app/src/task_comm.c` | Parser UART, lệnh queue, version, OTA REQUEST/OFF_ACK |
| `bootloader/src/main.c` | Entry point bootloader độc lập, giữ relay OFF |
| `bootloader/src/boot_protocol.c` | Packet, sequence, CRC, ACK/NACK, timeout |
| `bootloader/src/boot_flash.c` | Kiểm tra biên, erase/program/verify |
| `bootloader/src/boot_validate.c` | CRC/size/vector, ở lại recovery nếu firmware sai |
| `bootloader/src/boot_jump.c` | Dừng ngoại vi/ngắt, VTOR/MSP/clock handoff |
| `esp32/src/main.cpp` | Entry ESP32, Wi-Fi/web setup |
| `esp32/src/web_ota.cpp` + `esp32/data/index.html` | Upload, tiến độ/lỗi/phiên bản |
| `esp32/src/uart_bridge.cpp` | Gửi gói, retry, xác nhận app mới |
| `tests/manual/MANH.md` | Đo, I2C tranh chấp, UART/OTA, heap/stack |

Nền **đã chuẩn bị, chỉ kiểm tra/tích hợp, không giao viết lại**:
`bsp/*`, `app_ipc.*`, `app_cfg.h`, `app_types.h`, `rtos_hooks.c`, `main.c`,
`startup.c`, `FreeRTOSConfig.h`, Makefile, project Keil, GPIO/RCC/DMA/UART.
Bốn task phải do người nhận nhiệm vụ viết thật; bản khung chưa tạo task giả.

## Trình tự giao việc

1. Leader kiểm tra board/polarity, build cả môi trường và chạy khung trên kit.
2. Mạnh làm I2C trước; Võ có thể làm relay/button ngay; Công chốt model OLED và chuẩn bị UI/mock.
3. Code thiết bị trong `hal/` (relay/button/OLED/INA219) đã qua PR rồi mới viết task riêng, không chạm main.
4. Mạnh tích hợp trên `integration/system-v1`, bật từng `APP_ENABLE_*`, đo heap/stack và test.
5. Chức năng đo/điều khiển/hiển thị ổn mới ghép OTA. Chốt phân vùng Flash trước code bootloader.
6. Chỉ release vào main sau test phần cứng. Không gắn nhãn “ổn định” chỉ vì build được.
