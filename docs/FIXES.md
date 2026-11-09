# Các thay đổi từ MCU_Project gốc

## Cấu trúc và build

- `lib/gpio,rcc,dma,uart` -> `hal/inc/hal_*.h`, `hal/src/hal_*.c`; giữ tên hàm cũ khi có thể.
- Gộp thư mục `drivers/` cũ vào `hal/`: `ina219`, `oled`, `relay`, `button` nay nằm ở `hal/inc` + `hal/src`
  cạnh các file `hal_*`; bỏ include path `drivers/inc`, nhóm Keil "Drivers" nhập vào "HAL". Không đổi API.
- FreeRTOS nguyên gốc -> `middlewares/FreeRTOS-Kernel`; kernel V11.1.0 không sửa.
- SPI, TIM2 PWM, MAX7219 -> `extras/legacy/`, giữ mã gốc (chỉ đổi include), không build mặc định.
  Chúng chưa được nâng cấp thành driver dùng chung an toàn RTOS; có chờ polling vô hạn ở SPI cũ.
  PA0 trong TIM2 PWM cũ xung đột nút nhấn đề cương: không đưa module này vào sản phẩm hiện tại.
- Bỏ include `../../CMSIS` không đi kèm ZIP. Không giả bộ thêm CMSIS không tồn tại.
- Object/dependency/map/bin/hex vào `build/gcc`; Make + Keil dùng cùng manifest, đường dẫn tương đối.
- ARM Compiler 6 dùng cùng port FreeRTOS GCC/ARM_CM3 theo tài liệu nằm trong kernel gốc.
  Thêm scatter/nhánh startup `__main`; GCC giữ cơ chế copy data/zero bss.

## Lỗi thư viện cụ thể đã sửa

| Vị trí gốc | Vấn đề | Sửa |
|---|---|---|
| `Config_uart()` | Bật RXNEIE nhưng không enable NVIC IRQ37 | Set priority 6, clear pending, enable NVIC |
| UART CR3 | Bật DMAT thử nghiệm khi TX dùng interrupt ring | Tắt DMA trong mode interrupt |
| UART gửi chuỗi/số | Busy-wait vô hạn nếu TX không tiến; nhiều task tranh ring | Queue + mutex cả message + timeout hữu hạn; ISR FromISR |
| UART receive ISR | Không xử lý ORE/error độc lập | Đọc SR rồi DR, bỏ byte lỗi và đếm lỗi/drop |
| `UART_ReadLine` | max_size=0/1 và đổi buffer dễ sai biên; dòng quá dài có thể nhận đuôi thành lệnh mới | Bộ đệm nội bộ, guard kích thước, loại toàn dòng quá dài đến delimiter |
| `SendNumberInterrupt_UART` | Chỉ đặc cách 100, số 101..255 tạo ký tự sai | Chuyển đủ miền uint8 0..255 |
| DMA init | Xóa toàn CCR khi EN có thể đang bật; thiếu guard | Disable EN trước, kiểm config/channel/null |
| DMA start | Cờ completion/error cũ chưa xóa | Clear flags của channel trước enable lại |
| GPIO output config | speed=INPUT làm pin thành input dù yêu cầu output | Reject tốc độ không hợp lệ trước sửa CRL/CRH |
| FreeRTOS config | Tắt malloc/stack hooks và chưa có assert | Bật hooks/assert; lỗi force relay OFF rồi dừng để debug |

Đối chiếu UART/DMA với ST RM0008:
https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

## Thay đổi API cần biết

- Include `hal_uart.h` thay `uart.h`; tương tự `hal_gpio.h`, `hal_rcc.h`, `hal_dma.h`.
- `Config_uart()` trả `BaseType_t`: phải kiểm `pdPASS`, gọi trước scheduler đúng một lần.
- Các hàm `Send*Interrupt_UART` trả `hal_status_t`, timeout mặc định 1000ms; gọi từ task.
- `hal_uart_write/read` dùng timeout RTOS tick, cấm `portMAX_DELAY`; chỉ TX mutex hỗ trợ nhiều task.
- RX chỉ TaskComm tiêu thụ; `UART_ReadLine` là helper một consumer, không phải parser OTA.
- `HAL_OK` TX là đã enqueue, không bảo đảm wire idle; protocol phải chờ ACK.
- DMA vẫn là primitive một owner/channel, GPIO/RCC config chỉ init trước scheduler.

## Giới hạn

Sửa lỗi qua đọc code và kiểm tra ghi trong VALIDATION; chưa có kit để chứng minh timing điện,
relay polarity, UART mất byte thực tế hay I2C. Không tuyên bố toàn bộ thư viện đã được kiểm hết.
SPI/timer/MAX7219 cũ chỉ lưu lại, không giao viết lại, không gán nhãn production/RTOS-safe.
