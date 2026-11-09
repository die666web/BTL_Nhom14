# Các quyết định OTA chưa triển khai — Lê Trọng Mạnh

Đây là checklist thiết kế, chưa phải wire protocol đã hoạt động.

| Phải chốt trước code | Nội dung |
|---|---|
| Memory map | Boot/app/metadata trong 64 KiB; page size theo chip; đồng bộ linker GCC/Keil |
| Packet format | Magic, type, sequence, offset, payload length, payload, CRC; endian |
| CRC | Thuật toán, polynomial/init/reflection/xorout, test vector chung ESP/STM32 |
| Bắt tay | REQUEST/OFF_ACK ở app; boot ready; size/version; data; commit; reboot |
| Timeout/retry | Mỗi gói hữu hạn; duplicate sequence không ghi sai; mất UART có recovery |
| Hợp lệ app | Size/address, MSP trong SRAM, vector Thumb trong vùng app, CRC toàn ảnh |
| Mất nguồn | Một vùng app không rollback; metadata chỉ đánh valid sau verify; nhận lại nếu invalid |
| Thành công web | Chỉ sau ứng dụng mới gửi đúng phiên bản/xác nhận, không chỉ vì đã gửi xong |
| Clock/IRQ | Handoff về trạng thái thống nhất; VTOR đúng; dừng SysTick/DMA/IRQ trước jump |
| Firmware file | Binary từ đúng target có offset; không nhận firmware standalone hiện tại |

Bootloader không dùng FreeRTOS app; UART app đang dùng queue không được copy nguyên sang bootloader.
Tận dụng kiến thức thanh ghi, viết transport polling có timeout riêng trong boot_protocol khi đến phần OTA.
Không thêm tính năng SPI flash/rollback ở bản khởi đầu; đó là hướng mở rộng trong đề cương.
