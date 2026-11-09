# Kết quả kiểm tra bản khởi đầu — 09/11/2026

## Đã chạy

| Hạng mục | Kết quả |
|---|---|
| Build Linux ARM GCC 13.2.1, Cortex-M3 Thumb soft-float | PASS, 26 C files, compile + link + ELF/BIN/HEX |
| `-std=c11 -Wall -Wextra -Werror` | Không cảnh báo/lỗi trong build GCC cuối |
| Host regression bằng GCC + UndefinedBehaviorSanitizer | PASS, 564 assertions |
| `python3 tools/sync_projects.py --check` | PASS, Make + Keil chung danh sách 26 source |
| Project Keil | XML parse được, path source/include/scatter hợp lệ, ARM Compiler 6, MicroLIB tắt |
| Vector table từ ELF/BIN GCC | MSP 0x20005000; vector ở 0x08000000; Reset/SVC/PendSV/SysTick/USART1 đúng strong handler, Thumb bit |
| FreeRTOS vendored | 811 file giữ nguyên byte từ ZIP gốc, có license |
| Module giao việc | I2C, 4 driver, 4 task vẫn chỉ header và TODO; không stub trả thành công |

Kích thước bản GCC mặc định (chưa bật bốn task):

- `text = 10.100 bytes`, `data = 4 bytes`.
- `.bss = 10.696 bytes`, đã gồm FreeRTOS heap 10 KiB.
- `.heap_stack = 1.540 bytes`, gồm vùng dự phòng và alignment.
- GNU `size` báo `bss = 12.236 bytes` vì tính cả `.heap_stack` NOBITS.
- Flash image BIN = 10.104 bytes. Đây là bản khung; không suy ra đủ bộ nhớ cho sản phẩm hoàn chỉnh.

Lệnh chạy lại:

```bash
make check-projects
make -j4
sh tests/run_host_tests.sh
```

Host test dùng `cc` trên Linux, queue giả và vùng thanh ghi giả bằng mmap. Nó kiểm tra:
chuyển số 0..255, newline, queue đầy/timeout và nhả mutex, scheduler chưa chạy,
null/size 0/1, CRLF, dòng quá dài, lỗi/drop RX, GPIO bitfield và DMA config/flag/guard.
**Không mô phỏng thời gian điện, FreeRTOS scheduler hay tranh chấp ISR thực tế.**

## Chưa kiểm tra — không được coi là đã đạt

- Chưa chạy ARM Compiler 6/armlink thực tế trong Keil; chưa xác nhận load/debug/flash qua µVision.
  Kiểm XML/path không thay thế Rebuild All. Thành viên Windows cần chạy lần đầu và ghi kết quả.
- Chưa chạy trên Blue Pill: clock/tick, mức kích relay, mất UART thực tế, heap runtime/stack,
  hoạt động I2C, OLED, INA219.
- Chưa có logic sản phẩm hoặc OTA để test end-to-end. Bootloader/ESP32 chỉ là chỗ giao việc,
  chưa có build target; không được gọi là đã chạy trên cả ba MCU/project.
- Chưa test SPI/PWM/MAX7219 cũ (nằm ngoài build, không đổi logic).
- Chưa push repo, tạo branch/protection hay gắn release tag.

## Việc đầu tiên của nhóm

1. Mạnh kiểm baseline trên Blue Pill; xác nhận relay active level, OFF khi reset.
2. Một thành viên Windows mở Keil Compiler 6, Rebuild All và nạp/debug bản khung.
3. Sau đó nhận phần việc theo PHAN_CONG; mọi test kit ghi ở `tests/manual/`.
