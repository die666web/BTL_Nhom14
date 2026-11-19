# Đặc tả OTA qua UART giữa ESP32 và STM32F103C8T6 — v2 (bootloader ROM của ST)

> Code: `app/src/task_comm.c`, `bsp/bsp.c` (`bsp_system_init`, `bsp_reboot_to_bootloader`),
> `esp32/src/uart_bridge.cpp`. Quy trình kiểm tra: `docs/OTA_WORKFLOW.md`.

## 1. Ý tưởng
STM32F103 có sẵn một bootloader do ST nạp cứng trong chip (System Memory, `0x1FFFF000`).
Bootloader này nghe USART1 (PA9/PA10) và nhận lệnh theo giao thức **ST AN3155**.
Nhóm **không tự viết bootloader**: app tự nhảy vào bootloader ROM, ESP32 nói chuyện thẳng với nó.

Bản đồ Flash: app chiếm toàn bộ `0x08000000 – 0x08010000` (64 KiB). Không còn vùng bootloader riêng, không còn metadata.

## 2. UART
- Nối: ESP32 GPIO17 (TX) → PA10, GPIO16 (RX) ← PA9, chung GND, 115200 baud.
- Khi nói với **app**: 8N1.
- Khi nói với **bootloader ROM**: 8E1 (8 bit, parity chẵn, 1 stop) — bắt buộc theo AN3155.

## 3. Luồng một lần cập nhật

| Bước | ESP32 | STM32 |
|---|---|---|
| 1 | Gửi chuỗi `#ENTER_BOOTLOADER#` (8N1) | TaskComm nhận chuỗi, đặt `OTA_REQUEST` |
| 2 | | TaskControl khoá và tắt relay, đặt `RELAY_OFF_ACK` (TaskComm chờ tối đa 500 ms) |
| 3 | | `bsp_reboot_to_bootloader()`: relay OFF, ghi `0xB007` vào BKP_DR1, reset |
| 4 | | `bsp_system_init()`: relay OFF, thấy cờ → xoá cờ → nhảy vào `0x1FFFF000` |
| 5 | Đổi UART sang 8E1, gửi `0x7F` mỗi 100 ms (tối đa 3 s) tới khi nhận `0x79` | Bootloader ROM dò baud, trả ACK |
| 6 | Lệnh Erase `0x43` + `0xFF 0x00` (xoá cả Flash) | Xoá, trả ACK |
| 7 | Mỗi khối 256 B: Write Memory `0x31`, rồi Read Memory `0x11` đọc lại so sánh | Ghi / trả dữ liệu |
| 8 | Lệnh Go `0x21` tới `0x08000000` | Chạy app mới |
| 9 | UART về 8N1 | `bsp_system_init()` đặt VTOR, reset USART1 về mặc định |

Nếu chip trả NACK cho lệnh `0x43`, ESP32 thử Extended Erase `0x44` + `0xFF 0xFF 0x00`.

## 4. Giao thức AN3155 (phần dùng tới)
- ACK = `0x79`, NACK = `0x1F`.
- Mỗi lệnh: `[mã lệnh] [mã lệnh XOR 0xFF]` → chờ ACK.
- Địa chỉ: 4 byte (byte cao trước) + 1 byte checksum = XOR 4 byte → chờ ACK.
- Write Memory: lệnh → địa chỉ → `[N-1] [N byte dữ liệu] [checksum = XOR của N-1 và mọi byte]` → chờ ACK. N là bội của 4 (đệm `0xFF`).
- Read Memory: lệnh → địa chỉ → `[N-1] [(N-1) XOR 0xFF]` → ACK → N byte dữ liệu.
- Go: lệnh → địa chỉ → ACK, bootloader nhảy tới app.

## 5. An toàn
- Relay luôn OFF trong lúc nạp: TaskControl tắt trước khi reset, `bsp_system_init()` đặt PA0 output mức OFF trước khi nhảy vào bootloader ROM.
- ESP32 kiểm tra file trước khi xoá Flash STM32: kích thước ≤ 64 KiB, bảng vector (stack pointer trong SRAM, Reset_Handler trong `0x08000000 + size`).
- Mỗi khối ghi xong được đọc lại so sánh từng byte.
- Nạp hỏng giữa chừng (mất điện, rút dây): app không chạy được. Cứu bằng tay: gạt jumper **BOOT0 = 1**, reset, nạp lại từ web (ESP32 thấy bootloader đang chờ sẵn), xong gạt BOOT0 = 0.

## 6. Giới hạn
- Không còn bản firmware dự phòng trên STM32; bản trước vẫn giữ trên ESP32 (`/fw_prev.bin`) để nạp lại bằng tay.
- Bootloader ROM không biết relay: nếu module relay tự kích khi chân PA0 thả nổi thì phải thêm điện trở kéo xuống trên PA0.
