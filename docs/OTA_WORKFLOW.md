# Workflow OTA ESP32 → STM32 (v1) — hướng dẫn kiểm tra

Đặc tả: `docs/OTA_CONTRACT.md`. Tài liệu này dùng để **review code, build, nạp lần đầu và chạy các ca kiểm thử**.

> Trạng thái: code đã viết đủ hai phía nhưng **chưa build bằng ARM GCC/Keil, chưa chạy test PC, chưa thử trên board**.
> Mọi ô kết quả dưới đây đang chờ chạy thực tế.

---

## 1. Bức tranh tổng thể

```
 Trình duyệt ──HTTP──▶ ESP32 ──────────────── UART 115200 8N1 ───────────────▶ STM32F103C8
                        │  web_ota.cpp   (/upload /status /flash)              │
                        │  fw_store.cpp  (lưu /fw.bin, kiểm tra vector)        │  App (0x08002000)
                        │  uart_bridge.cpp (máy trạng thái gửi)  ── ENTER ──▶  │   task_comm.c → bsp_reboot_to_bootloader()
                        │                                                      │         │ BKP_DR1 = 0xB007, reset
                        │                                          ◀── READY ─ │  Bootloader (0x08000000)
                        │                                          ── BEGIN ─▶ │   boot_protocol.c (máy trạng thái)
                        │                                          ── DATA ──▶ │   boot_flash.c / boot_validate.c
                        │                                          ── END ───▶ │   commit metadata (0x0800FC00)
                        │                                          ◀── ACK ─── │   2 s → boot_jump.c → app mới
                        └── common/ota_frame (codec + CRC + layout) dùng chung cả 3 chương trình
```

## 2. Danh sách file (để review)

| Phần | File | Nội dung chính | Mới / sửa |
|---|---|---|---|
| Dùng chung | `common/ota_frame/ota_frame.{h,c}` | Frame A5 5A, CRC16/CRC32, builder, parser từng byte (timeout 20 ms) | Mới |
| | `common/ota_frame/ota_layout.h` | Địa chỉ boot/app/meta, SRAM, cờ 0xB007 | Mới |
| Bootloader | `bootloader/src/main.c` | Cờ BKP → app hợp lệ thì chạy, không thì chờ nạp | Viết lại |
| | `bootloader/src/boot_protocol.c` | WAIT_BEGIN → RECEIVING → COMPLETE_GRACE | Viết lại |
| | `bootloader/src/boot_validate.c` | Metadata, vector, CRC-32 ảnh | Viết lại |
| | `bootloader/src/boot_flash.c` | Xoá page, ghi half-word, đọc lại | Viết lại |
| | `bootloader/src/boot_port.c` + `inc/boot_hw.h` | SysTick, USART1 polling, LED PC13, relay OFF, BKP | Mới |
| | `bootloader/src/boot_jump.c` | Tắt IRQ, VTOR, MSP, nhảy app | Viết lại |
| | `bootloader/STM32F103C8_BOOT.ld`, `keil/Bootloader.sct`, `keil/Bootloader.uvprojx` | Linker/project bootloader | Mới |
| App | `app/src/task_comm.c` | Dò frame, nhận ENTER_UPDATE → handshake → reboot | Viết mới |
| | `bsp/bsp.c/.h` | `bsp_reboot_to_bootloader()` | Sửa |
| | `bsp/board_config.h` | UART 9600 → **115200** | Sửa |
| | `app/inc/app_cfg.h` | `APP_ENABLE_COMM = 1` | Sửa |
| | `STM32F103C8Tx_FLASH.ld`, `keil/STM32F103C8.sct`, `keil/MCU_Project.uvprojx` | App dời về **0x08002000, 55 KiB** | Sửa |
| Build | `Makefile`, `tools/sources.json`, `tools/bootloader_sources.json`, `tools/sync_projects.py`, `tools/make_meta.py` | Target `boot`, `flash-boot`, metadata lần đầu | Sửa / mới |
| ESP32 | `esp32/src/uart_bridge.cpp` | Máy trạng thái gửi, timeout, retry 3 lần | Viết mới |
| | `esp32/src/main.cpp`, `web_ota.cpp` | Wi-Fi AP + log, web server | Khôi phục |
| | `esp32/src/fw_store.cpp`, `include/stm32_memory_map.h` | Kiểm tra theo layout mới | Sửa |
| | `esp32/platformio.ini` | `lib_extra_dirs = ../common` | Sửa |
| Test | `tests/host/test_ota.c`, `tests/run_ota_tests.sh` | Codec + máy trạng thái bootloader trên Flash giả | Mới |

Gợi ý thứ tự đọc: `ota_frame.h` → `boot_protocol.c` → `uart_bridge.cpp` → `boot_validate.c` → `boot_flash.c` → `boot_jump.c` → `task_comm.c`.

## 3. Đấu dây

| ESP32 | STM32 Blue Pill |
|---|---|
| GPIO17 (TX) | PA10 (RX) |
| GPIO16 (RX) | PA9 (TX) |
| GND | GND |

BOOT0 = 0 (khởi động từ Flash). ST-Link nối SWD (SWDIO, SWCLK, GND, 3V3) để nạp lần đầu.
LED PC13: nháy chậm = bootloader chờ BEGIN, nháy nhanh = đang nhận, tắt = đang chạy app.

## 4. Build

### Linux / GCC
```bash
make sync-projects      # sinh lại tools/sources.mk + 2 project Keil
make                    # app (build/gcc/firmware.*) + bootloader (build/gcc-boot/bootloader.*)
sh tests/run_ota_tests.sh   # test PC
```
Kiểm tra trong output `size` và file `.map`:
- `bootloader.elf`: text + data **< 8192 B**, `.isr_vector` ở 0x08000000.
- `firmware.elf`: `.isr_vector` ở **0x08002000**, tổng Flash < 56 320 B.

### Keil
- `keil/Bootloader.uvprojx` → build → `build/keil/bootloader.axf`.
- `keil/MCU_Project.uvprojx` → build → `build/keil/firmware.axf`. Bật After Build `fromelf --bin` để có `firmware.bin` cho ESP32.
- Xem `.map`: vùng IROM đúng 0x08000000/0x2000 và 0x08002000/0xDC00.

### ESP32 (Windows, PlatformIO)
Mở thư mục `esp32/`, rồi:
```bash
pio run -t upload
```
```bash
pio run -t uploadfs
```

## 5. Nạp lần đầu (ST-Link)
1. Xoá toàn chip (tuỳ chọn): `st-flash erase`.
2. `make flash-boot` → bootloader ở 0x08000000. Reset: LED PC13 nháy chậm (chưa có app).
3. Chọn một trong hai:
   - **Qua ST-Link:** `make flash` → ghi app ở 0x08002000 + `meta.bin` ở 0x0800FC00. Reset: LED tắt, app chạy.
   - **Qua ESP32:** bỏ qua, nạp app bằng web ở bước 6 (bootloader đang chờ sẵn).

Keil: nạp `bootloader.axf` trước; app nạp qua ESP32 (hoặc tự chạy `python tools/make_meta.py build/keil/firmware.bin meta.bin` rồi ghi `meta.bin` ở 0x0800FC00).

## 6. Luồng OTA hằng ngày
1. Build app → lấy `firmware.bin` (đã link ở 0x08002000).
2. Điện thoại/laptop vào Wi-Fi ESP32 → `http://192.168.4.1`.
3. **Bước 1:** chọn `firmware.bin` → Tải lên. ESP32 kiểm tra kích thước, MSP, reset vector, lưu `/fw.bin`.
4. **Bước 2:** Nạp xuống STM32. Theo dõi trạng thái trên web và log USB của ESP32:
   ```
   [ota] dang vao bootloader (xxxx B, crc ........)
   [ota] bootloader san sang, dang xoa flash
   [ota] dang nap 0% ... 
   [ota] dang kiem tra tren STM32
   [ota] xong: firmware da qua kiem tra va duoc commit
   ```
5. Khoảng 2 s sau ACK(END), STM32 chạy app mới.

## 7. Ca kiểm thử (mục 8 của đặc tả)

| # | Ca | Cách thử | Kỳ vọng | Tự động (PC) | Board |
|---|---|---|---|---|---|
| 1 | Codec | `run_ota_tests.sh` | CRC khớp vector, 8 frame mẫu encode/decode đúng, magic trong payload vẫn 1 frame | `test_codec` | – |
| 2 | Mất byte / sai CRC / sai LEN | Test PC; trên board: rút dây RX giữa chừng | Không ghi từ frame hỏng, frame gửi lại được nhận | `test_codec`, `test_timeout_and_state` | ☐ |
| 3 | Mất READY / ACK(BEGIN) | Test PC | ENTER lặp lại → READY; BEGIN lặp lại → ACK, không xoá thêm | `test_lost_ready_and_begin_ack` | ☐ |
| 4 | Gói thường / gói cuối | File 1000 B (gói cuối 104), 1001 B (105, đệm FF) | CRC bỏ byte đệm, app hợp lệ | `test_sizes` | ☐ |
| 5 | ACK(DATA) mất / ACK cũ | Test PC | Gói trùng chỉ ACK lại, không ghi lặp | `test_duplicate_and_bad_data` | ☐ |
| 6 | Sai số / sai độ dài, CRC đúng | Test PC | BAD_SEQ / BAD_LENGTH trước khi ghi | `test_duplicate_and_bad_data` | – |
| 7 | BEGIN quá lớn | Test PC | BAD_IMAGE_SIZE, app + metadata cũ nguyên | `test_image_errors` | ☐ |
| 8 | Mất ACK(END) | Test PC | END lặp lại → ACK, commit đúng 1 lần | `test_duplicate_and_bad_data` | ☐ |
| 9 | Mất điện sau xoá / giữa DATA | Rút nguồn STM32 khi web báo "dang nap" | Bật lại: LED nháy chậm (ở bootloader), nạp lại từ đầu được | – | ☐ |
| 10 | CRC sai / vector sai | Test PC; board: upload file build ở 0x08000000 | ESP32 từ chối ngay; nếu lọt: BAD_VECTOR/IMAGE_CRC_ERROR, không commit | `test_image_errors` | ☐ |
| 11 | App chạy + build | Nạp OTA, quan sát app | App khởi động, FreeRTOS chạy, ENTER_UPDATE lần sau vẫn vào bootloader | – | ☐ |
| 12 | Timeout phiên 15 s | Test PC | Về WAIT_BEGIN, DATA sau đó BAD_STATE | `test_timeout_and_state` | ☐ |

Khi chạy trên board, ghi kết quả + log vào `tests/manual/MANH.md`.

## 8. Điểm cần chú ý khi review
- `boot_protocol.c`: xoá **metadata trước** rồi mới xoá app (`on_begin`); ghi `valid` **sau cùng** (`boot_meta_commit`).
- Gói trùng so với nội dung **đọc từ Flash** (`on_data`), không cần buffer RAM.
- `boot_jump.c`: đổi MSP và `bx` trong **cùng một khối asm**.
- `uart_bridge.cpp`: timeout đếm sau `Serial2.flush()` (byte cuối đã ra dây); ACK sai seq không reset bộ đếm.
- `task_comm.c`: app chỉ phản ứng với ENTER_UPDATE; frame khác bị bỏ qua.
- Ghi chú triển khai khác với bản đặc tả gốc: mục 8 của `docs/OTA_CONTRACT.md`.

## 9. Chưa làm / giới hạn v1
- Không rollback trên STM32 (một vùng app). `/fw_prev.bin` trên ESP32 chỉ được lưu, chưa tự nạp lại.
- Không xác thực chữ ký (CRC chỉ phát hiện lỗi).
- App chưa gửi version sau khi khởi động; "thành công" = có ACK(END).
- Timeout 100 ms cho DATA chưa đo trên board, chỉnh nếu thấy retry nhiều trong log.
