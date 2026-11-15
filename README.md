# HTNN2-N14 — bản khởi đầu STM32 dùng chung Linux + Keil

**Mục đích:** giao việc bằng file cụ thể; giữ thư viện đã có và sửa lỗi nền. Chưa phải sản phẩm hoàn chỉnh.

Mở [PHAN_CONG](docs/PHAN_CONG.md) trước khi viết code. Mỗi `.c` chưa làm đã có tên người,
phạm vi, API trong `.h`, TODO và điều kiện bàn giao. “Rỗng” ở đây là chưa có logic chức năng;
không tạo hàm giả báo thành công. File có sẵn không giao viết lại.

## Bắt đầu theo máy đang dùng

| Môi trường | Mở/build | Mã nguồn |
|---|---|---|
| Linux, GCC ARM | `make -j4` ở thư mục này | Chung `main.c`, `bsp/`, `hal/`, `app/`, `middlewares/` |
| Windows, Keil µVision 5 + ARM Compiler 6 | `keil/MCU_Project.uvprojx`, Rebuild | Chính các file ở cột trên, không copy vào thư mục Keil |

Đọc [BUILD_LINUX_KEIL](docs/BUILD_LINUX_KEIL.md) để cài toolchain, cấu hình compiler, nạp/debug.
Project Keil không dùng ARM Compiler 5. Compiler 6 dùng được cùng port GCC/ARM_CM3:
`middlewares/FreeRTOS-Kernel/portable/ARMClang/Use-the-GCC-ports.txt` là hướng dẫn trong thư viện gốc.

## Trạng thái bản này

- Có sẵn: startup, linker, GPIO/RCC/DMA, UART đã sửa, FreeRTOS V11.1.0, IPC, hook lỗi, pin map,
  cấu hình task, Makefile, project Keil, quy trình Git và bảng giao việc.
- I2C1 (`hal_i2c`) đã viết: thư viện chung cho mọi thiết bị trên bus, biên dịch sạch, **chưa test trên kit**.
- OTA v1 (bootloader + ESP32 web OTA + TaskComm nhận ENTER_UPDATE) đã viết, **chưa build/test trên kit**: `docs/OTA_WORKFLOW.md`.
- Chưa làm: INA219, relay/button driver, các task còn lại.
- Mặc định chỉ `APP_ENABLE_COMM = 1` (OTA), các task khác = 0: relay OFF theo cấu hình;
  chưa đo, chưa hiển thị, chưa nhận lệnh sản phẩm. TaskComm chỉ xử lý ENTER_UPDATE.
- Bật một task khi chưa viết hàm `task_*_create()` sẽ lỗi link; đây là dấu hiệu chưa hoàn thành,
  không “sửa” bằng hàm rỗng trả `pdPASS`.
- Flash: bootloader **0x08000000** (8 KiB), app **0x08002000** (55 KiB), metadata 0x0800FC00; RAM **20 KiB**.
  App phải được nạp sau bootloader (`make flash-boot` rồi `make flash`, hoặc qua ESP32).
- Relay tạm giả định **active-low**, nút PA0 nối GND/pull-up. Xác nhận relay và OLED thực tế trước khi đấu tải.

## Thư mục và quyền sửa

| Đường dẫn | Nội dung / người sửa |
|---|---|
| `bsp/` | Pin/clock, khởi động, OFF khẩn cấp; Mạnh |
| `hal/inc`, `hal/src` | Toàn bộ lớp phần cứng: ngoại vi MCU `hal_*` (GPIO/RCC/DMA/UART có sẵn, I2C của Mạnh) và thiết bị ngoài viết tiếp trên đó: INA219 (Mạnh), OLED (Công), relay/button (Võ) |
| `app/inc`, `app/src` | Task của từng người; IPC/config/types/hooks do Mạnh giữ |
| `middlewares/FreeRTOS-Kernel/` | Thư viện gốc, không giao thành viên viết lại |
| `main.c`, `startup.c`, `FreeRTOSConfig.h` | Tích hợp chung; chỉ Mạnh sửa |
| `keil/` | Project Compiler 6 và scatter file; không chứa bản sao source |
| `tools/` | Một manifest nguồn cho Make + Keil; leader quản lý |
| `bootloader/`, `esp32/`, `common/ota_frame/` | OTA v1: bootloader, ESP32 web OTA, codec dùng chung; Mạnh |
| `extras/legacy/` | SPI, PWM TIM2, MAX7219 cũ: giữ tham khảo, không build mặc định |
| `docs/`, `tests/manual/` | API, Git, test từng người, bằng chứng kiểm thử |

## Luồng dữ liệu đã thống nhất

Measure ghi mailbox `g_measurement_queue` → Display dùng `xQueuePeek`.
Control ghi mailbox `g_control_queue` → Display dùng `xQueuePeek`.
Comm gửi `app_command_t` → `g_command_queue` → Control.
OTA dùng event REQUEST/ACK riêng để không bị kẹt sau hàng đợi lệnh đầy.
I2C tự khóa mutex ở HAL, không khóa lần nữa ở INA219/OLED.

## Tài liệu phải đọc

1. [PHAN_CONG](docs/PHAN_CONG.md): đúng người, đúng file, việc đã có và chưa có.
2. [API_VA_TICH_HOP](docs/API_VA_TICH_HOP.md): dữ liệu, sở hữu tài nguyên, timeout, thứ tự ghép.
3. [GIT_WORKFLOW](docs/GIT_WORKFLOW.md): feature → develop → integration → main.
4. [FIXES](docs/FIXES.md): lỗi đã sửa và thay đổi API cũ.
5. [VALIDATION](docs/VALIDATION.md): kiểm tra thực tế và phần chưa kiểm tra.

Chỉ leader bật task trong `app/inc/app_cfg.h` sau khi PR đã được kiểm tra.
Việc tạo branch/repo/protection trên GitHub cần làm tại repo thật; gói ZIP không tự thiết lập được.

Hai PDF yêu cầu gốc có trong `docs/reference/` để cả nhóm đối chiếu.
