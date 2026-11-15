# ESP32 web OTA cho STM32
Lê Trọng Mạnh phụ trách. PlatformIO + Arduino, board `esp32dev`. Mở **thư mục `esp32/`** trong VS Code.
Đặc tả UART: `docs/OTA_CONTRACT.md`. Quy trình kiểm tra: `docs/OTA_WORKFLOW.md`.
Nối dây: ESP32 GPIO17 (TX) → STM32 PA10, GPIO16 (RX) ← PA9, chung GND, 115200 8N1.

## Các file

| File | Làm gì |
|---|---|
| `platformio.ini` | Board, framework, LittleFS, `lib_extra_dirs = ../common` (codec dùng chung với STM32) |
| `src/main.cpp` | Khởi động: LittleFS, Wi-Fi AP + log thiết bị kết nối, UART STM32, web |
| `src/web_ota.cpp` | Web server: `/`, `/status`, `/upload`, `/flash` |
| `data/index.html` | Giao diện web (nạp bằng `pio run -t uploadfs`) |
| `src/fw_store.cpp` | **Lưu LittleFS**: ghi file upload, CRC32, kiểm tra vector theo layout 0x08002000, giữ bản cũ |
| `src/uart_bridge.cpp` | Máy trạng thái gửi OTA v1: ENTER_UPDATE → BEGIN → DATA… → END, timeout + 3 lần gửi lại |
| `include/stm32_memory_map.h` | Lấy địa chỉ từ `common/ota_frame/ota_layout.h` |
| `include/secrets.h` | Tên + mật khẩu Wi-Fi (không commit, copy từ `.example`) |
| `../common/ota_frame/` | Frame A5 5A, CRC16/CRC32, parser — cùng code với bootloader và app |

## LittleFS lưu gì
```
/index.html    giao diện web (uploadfs)
/fw_new.bin    file đang nhận, lỗi thì xoá
/fw.bin        bản đã kiểm tra, sẽ nạp xuống STM32
/fw_prev.bin   bản trước (chưa tự rollback ở v1)
/fw.meta       "size crc" của /fw.bin
```
`uploadfs` ghi đè cả LittleFS, firmware đang lưu sẽ mất.

## Lệnh
PowerShell chưa có `pio` trong PATH thì dùng `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"` thay cho `pio`.
```
pio run -t upload      # nạp code
pio run -t uploadfs    # nạp data/ vào LittleFS
pio device monitor     # xem log (tắt trước khi nạp)
```
