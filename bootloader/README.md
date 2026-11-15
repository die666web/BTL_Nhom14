# Bootloader OTA STM32F103C8 — Lê Trọng Mạnh
Chương trình riêng ở **0x08000000 (8 KiB)**, không build chung với application. Đặc tả: `docs/OTA_CONTRACT.md`,
quy trình kiểm tra: `docs/OTA_WORKFLOW.md`.

| File | Việc | Phụ thuộc phần cứng |
|---|---|---|
| `src/main.c` | Đọc cờ BKP_DR1 → app hợp lệ thì chạy, không thì vào chế độ nạp | – |
| `src/boot_protocol.c` | Máy trạng thái WAIT_BEGIN → RECEIVING → COMPLETE_GRACE | Không (test được trên PC) |
| `src/boot_validate.c` | Metadata, kiểm tra vector, CRC-32 ảnh | Không |
| `src/boot_flash.c` | Xoá page / ghi half-word / đọc lại (PM0075) | Có |
| `src/boot_port.c` | SysTick 1 ms, USART1 polling 115200, LED PC13, relay OFF, cờ BKP | Có |
| `src/boot_jump.c` | Tắt IRQ/SysTick, VTOR, MSP, nhảy Reset_Handler app | Có |
| `../common/ota_frame/` | Codec frame + CRC dùng chung với app và ESP32 | Không |
| `STM32F103C8_BOOT.ld`, `../keil/Bootloader.sct` | Linker GCC / Keil | – |

Build: `make boot` (GCC) hoặc mở `keil/Bootloader.uvprojx`. Nạp: `make flash-boot` (ST-Link, 0x08000000).
Không dùng FreeRTOS, UART đọc polling (không ngắt), không log trên UART OTA.
LED PC13: nháy chậm = chờ BEGIN, nháy nhanh = đang nhận dữ liệu, tắt = đã chạy app.
