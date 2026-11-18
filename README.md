# HTNN2-N14 — Giám sát và điều khiển quạt DC (STM32F103 + FreeRTOS + ESP32 OTA)

STM32F103C8 (Blue Pill) đo điện áp/dòng quạt bằng INA219, hiển thị trên OLED, bật/tắt quạt qua relay
bằng nút nhấn. ESP32 làm web server để cập nhật firmware STM32 từ xa (OTA) qua UART,
dùng bootloader ROM có sẵn trong chip của ST (không tự viết bootloader).

## Phần cứng

| Thiết bị | Chân STM32 | Ghi chú |
|---|---|---|
| INA219 + OLED | I2C1: PB6 (SCL), PB7 (SDA) | Chung bus, địa chỉ 0x40 / 0x3C |
| Relay quạt | PA0 | Kích mức cao (đã xác nhận với module) |
| Nút nhấn | PB8 | Pull-up nội, nhấn nối GND |
| ESP32 | USART1: PA9 (TX), PA10 (RX) | 115200; 8N1 với app, 8E1 với bootloader ROM |

## Cấu trúc thư mục

| Thư mục | Nội dung |
|---|---|
| `main.c`, `startup.c`, `FreeRTOSConfig.h` | Khởi động, vector ngắt, cấu hình RTOS |
| `bsp/` | Pin map (`board_config.h`), khởi tạo board, tắt relay khẩn cấp, reset và nhảy vào bootloader ROM |
| `hal/` | Driver tự viết (bare-metal): GPIO, RCC, UART, I2C và thiết bị ngoài: relay, nút, INA219, OLED |
| `app/` | Các task FreeRTOS và IPC (queue, mutex, event group) |
| `esp32/` | Firmware ESP32 (PlatformIO): web upload, nạp STM32 theo giao thức AN3155 |
| `keil/` | Project Keil µVision (ARM Compiler 6) |
| `tests/` | Test PC (`tests/host`) và nhật ký test trên kit (`tests/manual`) |
| `docs/` | Tài liệu chi tiết |
| `middlewares/` | FreeRTOS Kernel V11.1.0 nguyên gốc |

## Kiến trúc phần mềm

```
TaskMeasure (200 ms) --g_measurement_queue--> TaskDisplay (400 ms) --> OLED
TaskControl (20 ms)  --g_control_queue------> TaskDisplay
Nút PB8 --> TaskControl --> relay (không có điều khiển relay từ xa)
TaskComm nhận "#ENTER_BOOTLOADER#" --> TaskControl tắt relay --> reset --> bootloader ROM của ST --> ESP32 nạp firmware mới
```

Bản đồ Flash: app chiếm cả `0x08000000` – `0x08010000` (64 KiB). Bootloader ROM nằm sẵn ở `0x1FFFF000`.

## Trạng thái

| Phần | Người | Trạng thái |
|---|---|---|
| HAL GPIO/RCC/UART, I2C1 | Mạnh | Đã viết, có test PC |
| Relay + nút nhấn + TaskControl | Võ | Đã viết, có test PC |
| OLED + TaskDisplay | Công | Đã viết, chờ test kit |
| OTA: ESP32 (AN3155) + TaskComm + BSP | Mạnh | Đã viết, chưa thử trên kit |
| INA219 + TaskMeasure | Mạnh | Đã viết |

Cả 4 task đã bật trong `app/inc/app_cfg.h`. `make ota-led` tạo app thử OTA (chỉ sáng LED PC13) để demo nạp qua web.

## Build và test nhanh

```bash
make            # app (cần arm-none-eabi-gcc)
make flash      # nạp app bằng ST-Link (0x08000000)
sh tests/run_tests.sh   # test PC trên Linux
```

Keil: mở `keil/MCU_Project.uvprojx`.

## Tài liệu

1. [PHAN_CONG](docs/PHAN_CONG.md) — ai làm file nào.
2. [API_VA_TICH_HOP](docs/API_VA_TICH_HOP.md) — phân tầng, tài nguyên, IPC.
3. [BUILD_LINUX_KEIL](docs/BUILD_LINUX_KEIL.md) — cài đặt, build, nạp.
4. [OTA_CONTRACT](docs/OTA_CONTRACT.md) và [OTA_WORKFLOW](docs/OTA_WORKFLOW.md) — đặc tả và cách chạy OTA.
5. [VALIDATION](docs/VALIDATION.md) — đã kiểm tra gì, chưa kiểm tra gì.
6. [GIT_WORKFLOW](docs/GIT_WORKFLOW.md) — quy trình nhánh feature → develop → main.

Tài liệu đề bài gốc: `docs/reference/`.
