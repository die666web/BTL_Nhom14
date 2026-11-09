# API và nguyên tắc tích hợp

## Phân tầng

- BSP: chân, clock và trạng thái an toàn của board.
- HAL (`hal/`): mọi code chạm phần cứng, gồm hai phần nằm chung thư mục:
  - Ngoại vi MCU, file `hal_*`: GPIO/RCC/DMA/UART/I2C. `hal_gpio.h` giữ tên hàm `GPIO_*` đã có, không buộc viết lại.
  - Thiết bị ngoài, file không có tiền tố `hal_`: `ina219`, `oled`, `relay`, `button`.
    Đây là code viết tiếp trên các hàm `hal_*` (ví dụ `ina219.c` gọi `hal_i2c`), **không phải task**:
    chỉ là hàm thư viện được task gọi, không tự chạy, không có vòng lặp/delay riêng.
    File thiết bị không được ghi thanh ghi trực tiếp; cần ngoại vi gì thì gọi `hal_*`.
- App: task, lịch chạy, dữ liệu/queue; không thao tác thanh ghi trực tiếp.
- Main: init một lần, tạo IPC/task, start scheduler; leader quản lý.

`hal/` là lớp tự viết bare-metal của nhóm, không phải STM32Cube HAL.
Mã không phụ thuộc đường dẫn CMSIS bên ngoài như Makefile cũ.

## Tài nguyên và hợp đồng

| Tài nguyên | Chủ sở hữu / cơ chế |
|---|---|
| PB8 relay | TaskControl duy nhất ghi trong vận hành; BSP force-off chỉ startup/panic |
| PA0 button | TaskControl gọi `button_poll_pressed` mỗi 20ms, không dùng TIM2_CH1 |
| I2C1 | HAL lấy g_i2c_mutex cho cả transaction (kể cả repeated START); không khóa lồng |
| INA219 | TaskMeasure duy nhất; không giữ mutex I2C khi delay chờ conversion |
| OLED | TaskDisplay duy nhất; địa chỉ 7-bit riêng, cùng bus |
| UART TX | `hal_uart_write` tự mutex toàn message, task context, timeout theo tick |
| UART RX | TaskComm duy nhất nhận; không đọc lẫn ReadLine/raw byte |
| DMA | Low-level, một owner cho mỗi channel; UART hiện không dùng DMA |
| GPIO_ConfigPin/RCC | Khởi tạo trước scheduler, không đổi pin/clock từ các task |
| Cấu hình RTOS/build | Mạnh giữ, hai compiler dùng cùng source |

UART hiện 9600 baud theo thư viện gốc. Có thể tăng khi test OTA, đổi một chỗ ở BSP và đồng bộ ESP32.
`HAL_OK` của write = đã vào queue; chưa bảo đảm bit cuối ra dây. Timeout có thể đã gửi một phần:
giao thức phải có length/CRC/ACK/retry, không coi lỗi là chưa gửi byte nào.
Không truyền `portMAX_DELAY`; không gọi UART task API từ ISR hay trước scheduler.
UART queue dùng API FromISR và IRQ priority 6 (numerical >=5). Không đổi priority grouping tùy tiện.

## IPC

`app_ipc_init()` đã tạo queue, mutex, event group; không tạo lại trong task.

- `g_measurement_queue`: 1 phần tử `app_measurement_t`; producer overwrite, reader peek.
- `g_control_queue`: 1 phần tử `app_control_state_t`; producer overwrite, reader peek.
- `g_command_queue`: 8 phần tử enum `app_command_t`; Comm gửi có timeout, báo lỗi khi đầy.
- `valid=false` và timestamp bảo đảm Display không giả vờ số đo cũ còn hợp lệ.
- Đơn vị nội bộ V/A/W; chỉ UI chuyển A → mA. Current có thể âm.
- Mỗi task gọi hàm init thiết bị của nó (`ina219_init`, `oled_init`...) một lần khi bắt đầu; I2C init chung ở main trước task.

## OTA handshake cần triển khai trong hai task

1. Comm clear `APP_EVENT_RELAY_OFF_ACK` cũ, set `APP_EVENT_OTA_REQUEST`.
2. Control kiểm tra REQUEST ưu tiên trước mọi ON/TOGGLE/nút; đặt khóa OTA, relay OFF,
   cập nhật mailbox rồi set OFF_ACK. Khi đã khóa, bỏ qua mọi yêu cầu bật.
3. Comm chờ OFF_ACK có timeout; chưa ACK thì không nhảy/reset.
4. Leader triển khai request bootloader bền qua reset và quy tắc thoát trạng thái OTA.

Không chờ ACK hoặc delay khi đang giữ mutex. Event ở đây dùng từ task, không từ ISR.
`APP_CMD_ENTER_OTA` nếu parser sử dụng phải chuyển thành cùng trình tự event, không tạo lối đi bỏ qua ACK.

## Tạo task

Hàm `prv...Task(void *)` và handle để `static` trong đúng file task.
Public API duy nhất là `BaseType_t task_xxx_create(void)` trả kết quả `xTaskCreate`.
Lấy stack/priority/period từ `app_cfg.h`; stack theo word 32 bit, không theo byte.
Task định kỳ dùng `vTaskDelayUntil` (`xTaskDelayUntil` backend của kernel này); UART wait có timeout.
Không return khỏi task; không dùng hardware delay hoặc vòng poll vô hạn.

## Tích hợp

Leader bật từng macro task sau khi có implementation. Source rỗng đã nằm sẵn trong cả hai project.
Không thêm task logic vào main. Không viết mock thành implementation chính thức.
Heap 10 KiB là ngân sách ban đầu, không chứng minh đủ khi thêm font/buffer/OTA:
đo `xPortGetFreeHeapSize`, minimum-ever heap và `uxTaskGetStackHighWaterMark` trên kit.
Không mở thêm timer/software queue không cần thiết mà không kiểm lại RAM.

## Thư viện I2C chung (`hal_i2c.h`)

Mọi driver thiết bị trên bus I2C1 chỉ gọi các hàm dưới, không chạm thanh ghi, không tự lấy mutex.
Địa chỉ 7 bit chưa dịch (`BSP_OLED_ADDR7` = 0x3C, `BSP_INA219_ADDR7` = 0x40).

| Hàm | Khung trên dây | Dùng cho |
|---|---|---|
| `hal_i2c_is_ready(addr, t)` | START, addr+W, STOP | Dò thiết bị, kiểm tra đấu dây |
| `hal_i2c_write(addr, buf, n, t)` | START, addr+W, buf, STOP | Gói tự soạn sẵn |
| `hal_i2c_read(addr, buf, n, t)` | START, addr+R, buf, STOP | Thiết bị không có thanh ghi |
| `hal_i2c_mem_write(addr, reg, buf, n, t)` | START, addr+W, reg, buf, STOP | INA219 ghi thanh ghi 16 bit; OLED lệnh (reg=0x00) / dữ liệu (reg=0x40) |
| `hal_i2c_mem_read(addr, reg, buf, n, t)` | START, addr+W, reg, Sr, addr+R, buf, STOP | INA219 đọc thanh ghi |

- `HAL_ERROR`: thiết bị NACK hoặc lỗi bus; `HAL_TIMEOUT`: hết giờ. Cả hai đã tự giải phóng bus, gọi lại được ngay.
- 100 kHz, polling: ~0.1 ms/byte, CPU bận trong lúc truyền. Task ưu tiên cao hơn vẫn chen vào được.
- Chỉ gọi từ task, sau `vTaskStartScheduler`; `hal_i2c_init()` gọi ở main.
