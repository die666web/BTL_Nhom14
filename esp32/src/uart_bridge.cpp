/* uart_bridge — nạp firmware xuống STM32 qua bootloader ROM có sẵn của ST. Lê Trọng Mạnh.
 * Giao thức: ST AN3155 "USART protocol used in the STM32 bootloader".
 *
 * Các bước:
 *   1. Gửi chuỗi "#ENTER_BOOTLOADER#" (UART 8N1) cho app STM32.
 *      App tắt relay, ghi cờ BKP, reset rồi nhảy vào bootloader ROM.
 *   2. Đổi UART sang 8E1 (bootloader ROM dùng parity chẵn), gửi 0x7F để bootloader dò baud,
 *      chờ ACK (0x79).
 *   3. Xoá toàn bộ Flash (lệnh Erase).
 *   4. Ghi từng khối 256 byte (lệnh Write Memory), đọc lại so sánh (lệnh Read Memory).
 *   5. Lệnh Go tới 0x08000000: STM32 chạy app mới.
 *
 * Việc nạp chạy trong một task FreeRTOS riêng, viết tuần tự từng bước cho dễ đọc;
 * web server ở loop() vẫn trả lời trạng thái trong lúc nạp. */
#include <Arduino.h>
#include <stdarg.h>
#include <LittleFS.h>
#include "uart_bridge.h"
#include "stm32_memory_map.h"

/* ---------- Nối dây và UART ---------- */
#define STM32_UART_BAUD       115200
#define STM32_UART_RX_PIN     16          /* ESP32 RX  <- STM32 PA9 (TX)  */
#define STM32_UART_TX_PIN     17          /* ESP32 TX  -> STM32 PA10 (RX) */

/* Chuỗi bảo app STM32 vào bootloader. PHẢI khớp ENTER_COMMAND trong app/src/task_comm.c. */
#define ENTER_COMMAND         "#ENTER_BOOTLOADER#"

/* ---------- Giao thức AN3155 ---------- */
#define BOOTLOADER_SYNC       0x7F        /* byte đầu tiên để bootloader dò baud */
#define BOOTLOADER_ACK        0x79
#define BOOTLOADER_NACK       0x1F

#define COMMAND_READ_MEMORY   0x11
#define COMMAND_GO            0x21
#define COMMAND_WRITE_MEMORY  0x31
#define COMMAND_ERASE         0x43
#define COMMAND_EXTENDED_ERASE 0x44       /* chip đời mới chỉ hỗ trợ lệnh này thay cho 0x43 */

#define BLOCK_SIZE            256         /* Write/Read Memory: tối đa 256 byte mỗi lần */

/* ---------- Thời gian chờ ---------- */
#define ACK_TIMEOUT_MS        1000
#define ERASE_TIMEOUT_MS      20000       /* xoá cả Flash mất vài giây */
#define SYNC_ATTEMPTS         30          /* 30 x 100 ms = 3 s chờ STM32 reset vào bootloader */
#define SYNC_RETRY_MS         100
#define ENTER_SETTLE_MS       100         /* chờ app reset xong sau khi gửi ENTER_COMMAND */

#define FLASH_TASK_STACK      8192

static HardwareSerial &stm32 = Serial2;

static volatile bool s_busy = false;
static char s_status[96] = "san sang";
static char s_firmware_path[32];


/* ================= Trạng thái hiển thị trên web ================= */

static void set_status(const char *format, ...)
{
    char text[sizeof(s_status)];
    va_list arguments;

    va_start(arguments, format);
    vsnprintf(text, sizeof(text), format, arguments);
    va_end(arguments);

    memcpy(s_status, text, sizeof(s_status));
    Serial.printf("[ota] %s\n", s_status);
}


/* ================= UART cấp thấp ================= */

static void open_uart(uint32_t config)
{
    stm32.end();
    stm32.begin(STM32_UART_BAUD, config, STM32_UART_RX_PIN, STM32_UART_TX_PIN);
}

static void clear_input(void)
{
    while (stm32.available())
    {
        stm32.read();
    }
}

/* Đọc 1 byte, trả -1 nếu hết thời gian. */
static int read_byte(uint32_t timeout_ms)
{
    uint32_t start = millis();

    while (millis() - start < timeout_ms)
    {
        if (stm32.available())
        {
            return stm32.read();
        }

        delay(1);
    }

    return -1;
}

static bool wait_ack(uint32_t timeout_ms)
{
    return read_byte(timeout_ms) == BOOTLOADER_ACK;
}


/* ================= Lệnh AN3155 ================= */

/* Mỗi lệnh gửi 2 byte: mã lệnh và phần bù của nó (XOR 0xFF). */
static bool send_command(uint8_t command)
{
    uint8_t frame[2] = { command, (uint8_t)(command ^ 0xFF) };

    stm32.write(frame, sizeof(frame));

    return wait_ack(ACK_TIMEOUT_MS);
}

/* Địa chỉ gửi 4 byte (byte cao trước) + 1 byte checksum = XOR của 4 byte. */
static bool send_address(uint32_t address)
{
    uint8_t frame[5];

    frame[0] = (uint8_t)(address >> 24);
    frame[1] = (uint8_t)(address >> 16);
    frame[2] = (uint8_t)(address >> 8);
    frame[3] = (uint8_t)(address);
    frame[4] = frame[0] ^ frame[1] ^ frame[2] ^ frame[3];

    stm32.write(frame, sizeof(frame));

    return wait_ack(ACK_TIMEOUT_MS);
}

/* Gửi 0x7F nhiều lần tới khi bootloader trả lời.
 * NACK cũng tính là sống: bootloader đã dò baud từ trước, giờ coi 0x7F là lệnh sai. */
static bool sync_with_bootloader(int attempts)
{
    for (int attempt = 0; attempt < attempts; attempt++)
    {
        clear_input();
        stm32.write(BOOTLOADER_SYNC);

        int reply = read_byte(SYNC_RETRY_MS);

        if (reply == BOOTLOADER_ACK || reply == BOOTLOADER_NACK)
        {
            return true;
        }
    }

    return false;
}

/* Xoá toàn bộ Flash. Thử lệnh Erase (STM32F1), không được thì dùng Extended Erase. */
static bool erase_all(void)
{
    if (send_command(COMMAND_ERASE))
    {
        uint8_t global_erase[2] = { 0xFF, 0x00 };          /* 0xFF = xoá tất cả, kèm checksum */

        stm32.write(global_erase, sizeof(global_erase));

        return wait_ack(ERASE_TIMEOUT_MS);
    }

    if (send_command(COMMAND_EXTENDED_ERASE))
    {
        uint8_t global_erase[3] = { 0xFF, 0xFF, 0x00 };    /* 0xFFFF = xoá tất cả, kèm checksum */

        stm32.write(global_erase, sizeof(global_erase));

        return wait_ack(ERASE_TIMEOUT_MS);
    }

    return false;
}

/* Ghi length byte (1..256, bội của 4) vào address. */
static bool write_block(uint32_t address, const uint8_t *data, size_t length)
{
    if (!send_command(COMMAND_WRITE_MEMORY))
    {
        return false;
    }

    if (!send_address(address))
    {
        return false;
    }

    /* Gửi (số byte - 1), dữ liệu, rồi checksum = XOR của tất cả các byte vừa gửi. */
    uint8_t count = (uint8_t)(length - 1);
    uint8_t checksum = count;

    for (size_t index = 0; index < length; index++)
    {
        checksum ^= data[index];
    }

    stm32.write(count);
    stm32.write(data, length);
    stm32.write(checksum);

    return wait_ack(ACK_TIMEOUT_MS);
}

/* Đọc length byte (1..256) từ address vào output. */
static bool read_block(uint32_t address, uint8_t *output, size_t length)
{
    if (!send_command(COMMAND_READ_MEMORY))
    {
        return false;
    }

    if (!send_address(address))
    {
        return false;
    }

    uint8_t count = (uint8_t)(length - 1);
    uint8_t frame[2] = { count, (uint8_t)(count ^ 0xFF) };

    stm32.write(frame, sizeof(frame));

    if (!wait_ack(ACK_TIMEOUT_MS))
    {
        return false;
    }

    for (size_t index = 0; index < length; index++)
    {
        int value = read_byte(ACK_TIMEOUT_MS);

        if (value < 0)
        {
            return false;
        }

        output[index] = (uint8_t)value;
    }

    return true;
}

/* Bảo bootloader nhảy tới address (chạy app). */
static bool go(uint32_t address)
{
    if (!send_command(COMMAND_GO))
    {
        return false;
    }

    return send_address(address);
}


/* ================= Quy trình nạp ================= */

/* Bước 1 + 2: đưa STM32 vào bootloader ROM và đồng bộ. */
static bool connect_to_bootloader(void)
{
    /* STM32 có thể đã ở sẵn bootloader (lần nạp trước lỗi, hoặc đã gạt BOOT0 = 1):
     * thử đồng bộ luôn. Nếu app đang chạy, mấy byte 0x7F này bị app bỏ qua. */
    open_uart(SERIAL_8E1);

    if (sync_with_bootloader(3))
    {
        return true;
    }

    /* App đang chạy: gửi lệnh bảo app vào bootloader. */
    open_uart(SERIAL_8N1);
    clear_input();
    stm32.print(ENTER_COMMAND);
    stm32.flush();

    /* Chờ app tắt relay + reset vào bootloader (thường < 50 ms), rồi mới đồng bộ:
     * byte 0x7F đầu tiên phải tới khi bootloader đã sẵn sàng thì nó mới dò baud đúng. */
    open_uart(SERIAL_8E1);
    delay(ENTER_SETTLE_MS);

    return sync_with_bootloader(SYNC_ATTEMPTS);
}

/* Bước 4: ghi cả file theo từng khối 256 byte, mỗi khối đọc lại so sánh. */
static bool write_and_verify_file(File &file, uint32_t file_size)
{
    uint8_t block[BLOCK_SIZE];
    uint8_t readback[BLOCK_SIZE];
    uint32_t offset = 0;

    while (offset < file_size)
    {
        size_t length = file.read(block, BLOCK_SIZE);

        if (length == 0)
        {
            set_status("loi: doc file LittleFS");
            return false;
        }

        /* Write Memory cần số byte là bội của 4: đệm 0xFF (giống Flash đã xoá). */
        while (length % 4 != 0)
        {
            block[length] = 0xFF;
            length++;
        }

        uint32_t address = STM32_APP_START + offset;

        if (!write_block(address, block, length))
        {
            set_status("loi: ghi Flash tai 0x%08lX", (unsigned long)address);
            return false;
        }

        if (!read_block(address, readback, length) || memcmp(block, readback, length) != 0)
        {
            set_status("loi: doc lai khong khop tai 0x%08lX", (unsigned long)address);
            return false;
        }

        offset += BLOCK_SIZE;

        if ((offset / BLOCK_SIZE) % 16 == 0)
        {
            uint32_t done = offset < file_size ? offset : file_size;

            set_status("dang nap %lu%% (%lu/%lu B)",
                       (unsigned long)(done * 100 / file_size),
                       (unsigned long)done, (unsigned long)file_size);
        }
    }

    return true;
}

static bool flash_firmware(void)
{
    File file = LittleFS.open(s_firmware_path, "r");

    if (!file)
    {
        set_status("loi: khong mo duoc %s", s_firmware_path);
        return false;
    }

    uint32_t file_size = file.size();

    if (file_size < 8 || file_size > STM32_APP_MAX_SIZE)
    {
        set_status("loi: kich thuoc %lu B ngoai vung Flash", (unsigned long)file_size);
        file.close();
        return false;
    }

    set_status("dang vao bootloader STM32");

    if (!connect_to_bootloader())
    {
        set_status("loi: STM32 khong vao bootloader (kiem tra day UART)");
        file.close();
        return false;
    }

    set_status("dang xoa Flash");

    if (!erase_all())
    {
        set_status("loi: xoa Flash");
        file.close();
        return false;
    }

    bool written = write_and_verify_file(file, file_size);

    file.close();

    if (!written)
    {
        return false;              /* set_status đã ghi lý do */
    }

    if (!go(STM32_APP_START))
    {
        set_status("loi: lenh Go (firmware da ghi xong, reset STM32 de chay)");
        return false;
    }

    set_status("xong: da nap va doc lai dung %lu B, STM32 dang chay app moi",
               (unsigned long)file_size);
    return true;
}

static void flash_task(void *argument)
{
    (void)argument;

    flash_firmware();

    open_uart(SERIAL_8N1);         /* trả UART về chế độ nói chuyện với app */
    s_busy = false;

    vTaskDelete(NULL);
}


/* ================= API ================= */

void uart_bridge_begin(void)
{
    open_uart(SERIAL_8N1);
}

bool uart_bridge_busy(void)
{
    return s_busy;
}

const char *uart_bridge_state_text(void)
{
    return s_status;
}

bool uart_bridge_start(const char *path)
{
    if (s_busy)
    {
        return false;
    }

    snprintf(s_firmware_path, sizeof(s_firmware_path), "%s", path);
    s_busy = true;
    set_status("bat dau nap %s", s_firmware_path);

    if (xTaskCreate(flash_task, "stm32_flash", FLASH_TASK_STACK, NULL, 1, NULL) != pdPASS)
    {
        s_busy = false;
        set_status("loi: khong tao duoc task nap");
        return false;
    }

    return true;
}
