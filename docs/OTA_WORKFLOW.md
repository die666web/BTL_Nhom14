# Workflow OTA ESP32 → STM32 (v2, bootloader ROM của ST) — hướng dẫn kiểm tra

Đặc tả: `docs/OTA_CONTRACT.md`.

## 1. Bức tranh tổng thể
```
Trình duyệt ──Wi-Fi──> ESP32 (lưu /fw.bin) ──UART──> STM32
                                   │
   1. "#ENTER_BOOTLOADER#" ──────> app: tắt relay, ghi cờ BKP, reset, nhảy vào bootloader ROM
   2. 0x7F / Erase / Write+Read / Go ──> bootloader ROM của ST (AN3155) ghi Flash, chạy app mới
```

## 2. Danh sách file (để review)
| File | Việc |
|---|---|
| `app/src/task_comm.c` | Dò chuỗi `#ENTER_BOOTLOADER#`, bắt tay tắt relay với TaskControl, gọi reset |
| `bsp/bsp.c` | `bsp_reboot_to_bootloader()` ghi cờ + reset; `bsp_system_init()` thấy cờ thì nhảy vào `0x1FFFF000` |
| `esp32/src/uart_bridge.cpp` | Giao thức AN3155: đồng bộ, xoá, ghi + đọc lại, Go |
| `esp32/src/fw_store.cpp` | Lưu file upload, kiểm tra kích thước và bảng vector |
| `esp32/src/web_ota.cpp`, `esp32/data/index.html` | Web upload và nút nạp |

Thứ tự đọc gợi ý: `task_comm.c` → `bsp.c` → `uart_bridge.cpp` (từ `flash_firmware()` đọc xuống).

## 3. Đấu dây
| ESP32 | STM32 Blue Pill |
|---|---|
| GPIO17 (TX) | PA10 (RX) |
| GPIO16 (RX) | PA9 (TX) |
| GND | GND |

BOOT0 = 0 khi chạy bình thường. ST-Link nối SWD để nạp lần đầu.

## 4. Build
- GCC: `make` → `build/gcc/firmware.bin`, `.isr_vector` ở **0x08000000**, tổng Flash < 64 KiB.
- Keil: `keil/MCU_Project.uvprojx` → build. Bật After Build `fromelf --bin` để có `firmware.bin` cho ESP32.
- ESP32 (mở thư mục `esp32/`): `pio run -t upload`, rồi `pio run -t uploadfs`.

## 5. Nạp lần đầu
`make flash` (hoặc Keil Download) ghi app ở 0x08000000. Không còn bước nạp bootloader riêng.

## 6. Luồng OTA hằng ngày
1. Kết nối Wi-Fi của ESP32, mở `http://192.168.4.1`.
2. Tải `firmware.bin` lên (ESP32 kiểm tra kích thước, bảng vector).
3. Bấm "Nạp xuống STM32". Trạng thái lần lượt: `dang vao bootloader` → `dang xoa Flash` → `dang nap x%` → `xong`.
4. STM32 tự chạy app mới (lệnh Go). Kiểm tra OLED báo đúng phiên bản.

## 7. Ca kiểm thử trên kit
| Ca | Kỳ vọng |
|---|---|
| Nạp khi quạt đang ON | Relay tắt trước khi STM32 reset, giữ OFF suốt lúc nạp |
| File sai địa chỉ (build cho 0x08002000) | ESP32 từ chối ngay khi upload, STM32 không bị xoá |
| Rút dây UART giữa lúc nạp | Web báo lỗi; gạt BOOT0 = 1, reset, nạp lại được |
| Nạp liên tiếp hai lần | Lần hai vẫn vào được bootloader (app mới vẫn có TaskComm) |
| App test LED (`make ota-led`) | LED PC13 sáng liên tục sau khi nạp; nạp lại firmware thật được |

## 8. Điểm cần chú ý khi review
- `bsp_system_init()` phải tắt relay **trước** khi nhảy vào bootloader ROM.
- Sau lệnh Go, bootloader ROM để USART1 ở 8E1: `bsp_system_init()` reset USART1 trước khi `Config_uart()`.
- `ENTER_COMMAND` ở `task_comm.c` và `uart_bridge.cpp` phải giống hệt nhau.

## 9. Giới hạn
Xem `docs/OTA_CONTRACT.md` mục 6.
