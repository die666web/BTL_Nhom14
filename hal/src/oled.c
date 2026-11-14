/* CHƯA TRIỂN KHAI — Vũ Chí Công
 * File dành riêng cho phần việc này, không phải module đã chạy.
 * Đọc docs/PHAN_CONG.md và header cùng tên trước khi viết.
 * Xác nhận IC/kích thước màn thực tế trước code (đề cương chưa chốt SSD1306). Dùng hal_i2c_write và timeout; font/UI nằm đây. Chỉ TaskDisplay sử dụng, không khóa g_i2c_mutex bên ngoài HAL.
 * Điền các hàm khai báo trong header; helper nội bộ để static.
 * Không sửa main.c, BSP, FreeRTOSConfig.h hoặc API chung trong nhánh này.
 * Không dùng delay_ms/HAL_Delay trong task, không chờ vô hạn.
 * Khi xong: test riêng trên kit, ghi kết quả vào tests/manual/, mở PR -> develop.
 */

/* TODO: viết phần triển khai ở đây. Không có stub giả trả về thành công. */

#include "oled.h"
#include "hal_i2c.h"
#include "board_config.h"
#include <stdio.h>

#define OLED_WIDTH       128U
#define OLED_HEIGHT       64U
#define OLED_PAGES         8U
#define OLED_BUFFER_SIZE 1024U

static uint8_t oled_buffer[OLED_BUFFER_SIZE];

hal_status_t oled_init(void)
{
    // Các lệnh khởi tạo SSD1306 128x64
    static const uint8_t init_cmd[] = {
        0xAE,       // Tắt hiển thị
        0xD5, 0x80, // Clock
        0xA8, 0x3F, // Multiplex: 64 hàng
        0xD3, 0x00, // Display offset = 0
        0x40,       // Start line = 0
        0x8D, 0x14, // Bật charge pump
        0x20, 0x00, // Horizontal addressing
        0xA1,       // Segment remap
        0xC8,       // COM scan direction
        0xDA, 0x12, // COM pins cho 128x64
        0x81, 0xCF, // Contrast
        0xD9, 0xF1, // Pre-charge
        0xDB, 0x40, // VCOMH
        0xA4,       // Hiển thị theo RAM
        0xA6,       // Normal display
        0xAF        // Bật hiển thị
    };

    hal_status_t status;

    // Kiểm tra OLED có phản hồi không
    status = hal_i2c_is_ready(
        BSP_OLED_ADDR7,
        pdMS_TO_TICKS(100)
    );

    if (status != HAL_OK)
    {
        return status;
    }

    // Gửi các lệnh khởi tạo OLED
    status = hal_i2c_mem_write(
        BSP_OLED_ADDR7,
        0x00,
        init_cmd,
        sizeof(init_cmd),
        pdMS_TO_TICKS(100)
    );

    return status;
}


static hal_status_t oled_flush(void)
{
    static const uint8_t addr_cmd[] = {
        0x21, 0x00, 0x7F, // Column 0 -> 127
        0x22, 0x00, 0x07  // Page 0 -> 7
    };

    hal_status_t status;

    // Chọn toàn bộ vùng hiển thị
    status = hal_i2c_mem_write(
        BSP_OLED_ADDR7,
        0x00,
        addr_cmd,
        sizeof(addr_cmd),
        pdMS_TO_TICKS(100)
    );

    if (status != HAL_OK)
    {
        return status;
    }

    // Truyền 1024 byte pixel
    return hal_i2c_mem_write(
        BSP_OLED_ADDR7,
        0x40,
        oled_buffer,
        sizeof(oled_buffer),
        pdMS_TO_TICKS(200)
    );
}


typedef struct {
    char character;
    uint8_t columns[5];
} oled_glyph_t;

// Font 5x7 tối thiểu để thử "HELLO STM32"

static const oled_glyph_t oled_font[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00}},
    {'-', {0x08, 0x08, 0x08, 0x08, 0x08}},
    {'.', {0x00, 0x60, 0x60, 0x00, 0x00}},
    {':', {0x00, 0x36, 0x36, 0x00, 0x00}},
    {'?', {0x02, 0x01, 0x51, 0x09, 0x06}},

    {'0', {0x3E, 0x45, 0x49, 0x51, 0x3E}},
    {'1', {0x00, 0x42, 0x7F, 0x40, 0x00}},
    {'2', {0x62, 0x51, 0x49, 0x49, 0x46}},
    {'3', {0x22, 0x41, 0x49, 0x49, 0x36}},
    {'4', {0x18, 0x14, 0x12, 0x7F, 0x10}},
    {'5', {0x2F, 0x49, 0x49, 0x49, 0x31}},
    {'6', {0x3E, 0x49, 0x49, 0x49, 0x32}},
    {'7', {0x01, 0x71, 0x09, 0x05, 0x03}},
    {'8', {0x36, 0x49, 0x49, 0x49, 0x36}},
    {'9', {0x26, 0x49, 0x49, 0x49, 0x3E}},

    {'A', {0x7E, 0x11, 0x11, 0x11, 0x7E}},
    {'B', {0x7F, 0x49, 0x49, 0x49, 0x36}},
    {'C', {0x3E, 0x41, 0x41, 0x41, 0x22}},
    {'D', {0x7F, 0x41, 0x41, 0x22, 0x1C}},
    {'E', {0x7F, 0x49, 0x49, 0x49, 0x41}},
    {'F', {0x7F, 0x09, 0x09, 0x09, 0x01}},
    {'G', {0x3E, 0x41, 0x49, 0x49, 0x7A}},
    {'H', {0x7F, 0x08, 0x08, 0x08, 0x7F}},
    {'I', {0x00, 0x41, 0x7F, 0x41, 0x00}},
    {'J', {0x20, 0x40, 0x41, 0x3F, 0x01}},
    {'K', {0x7F, 0x08, 0x14, 0x22, 0x41}},
    {'L', {0x7F, 0x40, 0x40, 0x40, 0x40}},
    {'M', {0x7F, 0x02, 0x0C, 0x02, 0x7F}},
    {'N', {0x7F, 0x02, 0x0C, 0x10, 0x7F}},
    {'O', {0x3E, 0x41, 0x41, 0x41, 0x3E}},
    {'P', {0x7F, 0x09, 0x09, 0x09, 0x06}},
    {'Q', {0x3E, 0x41, 0x51, 0x21, 0x5E}},
    {'R', {0x7F, 0x09, 0x19, 0x29, 0x46}},
    {'S', {0x46, 0x49, 0x49, 0x49, 0x31}},
    {'T', {0x01, 0x01, 0x7F, 0x01, 0x01}},
    {'U', {0x3F, 0x40, 0x40, 0x40, 0x3F}},
    {'V', {0x1F, 0x20, 0x40, 0x20, 0x1F}},
    {'W', {0x7F, 0x20, 0x18, 0x20, 0x7F}},
    {'X', {0x63, 0x14, 0x08, 0x14, 0x63}},
    {'Y', {0x07, 0x08, 0x70, 0x08, 0x07}},
    {'Z', {0x61, 0x51, 0x49, 0x45, 0x43}},

    {'m', {0x7C, 0x04, 0x18, 0x04, 0x78}}
};


static void oled_draw_char(uint8_t x,
                           uint8_t page,
                           char ch)
{
		if (ch >= 'a' && ch <= 'z' && ch != 'm')
{
    ch = (char)(ch - 'a' + 'A');
}
	
    if (page >= OLED_PAGES ||
        (uint16_t)x + 5U > OLED_WIDTH)
        return;

    for (size_t i = 0;
         i < sizeof(oled_font) / sizeof(oled_font[0]);
         i++)
    {
        if (oled_font[i].character == ch)
        {
            for (uint8_t col = 0; col < 5U; col++)
            {
                oled_buffer[page * OLED_WIDTH + x + col]
                    = oled_font[i].columns[col];
            }
            return;
        }
    }
}

static void oled_draw_text(uint8_t x,
                           uint8_t page,
                           const char *text)
{
    if (text == NULL || page >= OLED_PAGES)
        return;

    while (*text != '\0' &&
           (uint16_t)x + 5U <= OLED_WIDTH)
    {
        oled_draw_char(x, page, *text);
        text++;
        x = (uint8_t)(x + 6U);
    }
}



static void oled_format_value(
    char *out,
    size_t size,
    char label,
    float value,
    float scale,
    uint32_t divisor,
    unsigned decimals,
    const char *unit)
{
    if (out == NULL || size == 0U)
        return;

    // Kiểm tra NaN và phạm vi chuyển đổi
    float abs_value = value < 0.0f ? -value : value;
    float scaled = abs_value * scale + 0.5f;

    if (!(scaled >= 0.0f && scaled < 100000000.0f))
    {
        (void)snprintf(out, size, "%c:ERR", label);
        return;
    }

    uint32_t n = (uint32_t)scaled;

    (void)snprintf(
        out, size,
        "%c:%s%lu.%0*lu%s",
        label,
        value < 0.0f ? "-" : "",
        (unsigned long)(n / divisor),
        (int)decimals,
        (unsigned long)(n % divisor),
        unit
    );
}



hal_status_t oled_show(
    const app_measurement_t *measurement,
    const app_control_state_t *control,
    const char *version)
{
    if (measurement == NULL ||
        control == NULL ||
        version == NULL)
    {
        return HAL_INVALID_ARGUMENT;
    }

    char line[40];

    // Xóa framebuffer cũ
    for (size_t i = 0; i < OLED_BUFFER_SIZE; i++)
    {
        oled_buffer[i] = 0U;
    }

    oled_draw_text(0, 0, "POWER MONITOR");

    if (measurement->valid)
    {
        // Điện áp: 2 chữ số thập phân
        oled_format_value(
            line, sizeof(line), 'U',
            measurement->voltage_v,
            100.0f, 100U, 2U, "V"
        );
        oled_draw_text(0, 1, line);

        // Dòng điện: A -> mA
        oled_format_value(
            line, sizeof(line), 'I',
            measurement->current_a,
            10000.0f, 10U, 1U, "mA"
        );
        oled_draw_text(0, 2, line);

        // Công suất: 3 chữ số thập phân
        oled_format_value(
            line, sizeof(line), 'P',
            measurement->power_w,
            1000.0f, 1000U, 3U, "W"
        );
        oled_draw_text(0, 3, line);
    }
    else
    {
        oled_draw_text(0, 1, "SENSOR ERROR");
    }

    if (control->relay_on)
        oled_draw_text(0, 4, "RELAY:ON");
    else
        oled_draw_text(0, 4, "RELAY:OFF");

    if (control->ota_locked)
    {
        oled_draw_text(0, 5, "OTA LOCK");
    }
    else
    {
        (void)snprintf(
            line, sizeof(line),
            "VER:%s", version
        );
        oled_draw_text(0, 5, line);
    }

    return oled_flush();
}




