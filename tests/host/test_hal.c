/* Test PC cho lớp HAL: UART, GPIO, relay (PA0), nút nhấn (PB8).
 * Chạy: sh tests/run_tests.sh (Linux, cần gcc).
 *
 * Cách giả lập: vùng thanh ghi ngoại vi (0x40000000) và NVIC (0xE000E000) được mmap
 * thành RAM thường, FreeRTOS queue/mutex được thay bằng bản giả đơn giản bên dưới.
 * Test chỉ kiểm tra logic ghi/đọc thanh ghi, KHÔNG mô phỏng thời gian điện hay ngắt thật. */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "hal_uart.h"
#include "hal_gpio.h"
#include "relay.h"
#include "button.h"
#include "board_config.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

static unsigned checks;
#define CHECK(x) do { if (!(x)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); assert(0); } ++checks; } while (0)

/* ---------------- FreeRTOS giả: queue vòng + mutex cờ ---------------- */
struct MockQueue { size_t count, width, used, head, tail; unsigned char *data; };
struct MockMutex { int locked; };
static TickType_t now;
TickType_t xTaskGetTickCount(void) { return now; }
BaseType_t xTaskGetSchedulerState(void) { return taskSCHEDULER_RUNNING; }
QueueHandle_t xQueueCreate(UBaseType_t count, UBaseType_t size)
{
    QueueHandle_t q = calloc(1, sizeof(*q));
    q->count = count; q->width = size; q->data = calloc(count, size);
    return q;
}
void vQueueDelete(QueueHandle_t q) { free(q->data); free(q); }
BaseType_t xQueueSend(QueueHandle_t q, const void *d, TickType_t timeout)
{
    if (q->used == q->count) { now += timeout; return pdFAIL; }   /* đầy: "chờ" hết timeout */
    memcpy(q->data + q->head * q->width, d, q->width);
    q->head = (q->head + 1) % q->count; q->used++;
    return pdPASS;
}
BaseType_t xQueueReceive(QueueHandle_t q, void *d, TickType_t timeout)
{
    if (!q->used) { now += timeout; return pdFAIL; }
    memcpy(d, q->data + q->tail * q->width, q->width);
    q->tail = (q->tail + 1) % q->count; q->used--;
    return pdPASS;
}
BaseType_t xQueueSendFromISR(QueueHandle_t q, const void *d, BaseType_t *w) { (void)w; return xQueueSend(q, d, 0); }
BaseType_t xQueueReceiveFromISR(QueueHandle_t q, void *d, BaseType_t *w) { (void)w; return xQueueReceive(q, d, 0); }
SemaphoreHandle_t xSemaphoreCreateMutex(void) { return calloc(1, sizeof(struct MockMutex)); }
BaseType_t xSemaphoreTake(SemaphoreHandle_t m, TickType_t t) { (void)t; if (m->locked) return pdFAIL; m->locked = 1; return pdPASS; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t m) { m->locked = 0; return pdPASS; }
void vSemaphoreDelete(SemaphoreHandle_t m) { free(m); }

/* ---------------- Thanh ghi USART1 giả ---------------- */
static volatile uint32_t *uart = (volatile uint32_t *)0x40013800UL; /* [0]=SR [1]=DR [2]=BRR [3]=CR1 */

static void map_region(uintptr_t addr, size_t size)
{
    void *p = mmap((void *)addr, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    assert(p != (void *)-1);
}
/* Giả một byte đến chân RX: đặt cờ RXNE rồi gọi ISR. */
static void feed_text(const char *s)
{
    while (*s) { uart[0] = 1U << 5; uart[1] = (unsigned char)*s++; USART1_IRQHandler(); }
}
/* Chạy ISR với cờ TXE cho tới khi TXEIE tắt, gom các byte ISR đã ghi vào DR.
 * DR đặt giá trị mốc 0x100 (không phải byte) trước mỗi lần gọi để biết ISR có ghi hay không. */
static void drain(char *out, size_t cap)
{
    size_t n = 0;
    while (uart[3] & (1U << 7)) {
        uart[0] = 1U << 7; uart[1] = 0x100U; USART1_IRQHandler();
        if (uart[1] != 0x100U) { assert(n + 1 < cap); out[n++] = (char)uart[1]; }
    }
    out[n] = 0;
}

/* ===================== UART ===================== */

/* 1. Khởi tạo: BRR = 115200 @ 8 MHz (69), bật UE/TE/RE/RXNEIE, không bật DMA, bật IRQ37. */
static void test_uart_init(void)
{
    Config_uart();
    CHECK(uart[2] == (BSP_APB2_CLOCK_HZ + BSP_UART_BAUD / 2U) / BSP_UART_BAUD);
    CHECK(uart[2] == 69U);
    CHECK((uart[3] & ((1U << 2) | (1U << 3) | (1U << 5) | (1U << 13))) ==
          ((1U << 2) | (1U << 3) | (1U << 5) | (1U << 13)));
    CHECK(uart[5] == 0U);
    CHECK((*(volatile uint32_t *)0xE000E104UL) == (1U << 5));
}

/* 2. Gửi số: đủ 0..255 ra đúng chuỗi (bản cũ in sai từ 101 trở lên). */
static void test_uart_send_number(void)
{
    char out[16], expected[8];
    for (unsigned n = 0; n <= 255U; n++) {
        snprintf(expected, sizeof expected, "%u", n);
        SendNumberInterrupt_UART((uint8_t)n);
        drain(out, sizeof out);
        CHECK(strcmp(out, expected) == 0);
    }
}

/* 3. Nhận một dòng kết thúc CRLF -> trả đúng nội dung, không còn dòng thừa.
 *    UART_ReadLine xử lý 1 byte mỗi lần gọi nên gọi lặp tới khi có dòng. */
static void test_uart_read_line(void)
{
    char line[16];
    unsigned calls = 0;
    feed_text("HELLO\r\n");
    while (!UART_ReadLine(line, sizeof line)) CHECK(++calls < 16U);
    CHECK(strcmp(line, "HELLO") == 0);
    CHECK(UART_ReadLine(line, sizeof line) == 0);   /* '\n' sau '\r' không tạo dòng rỗng */
    CHECK(UART_ReadLine(line, sizeof line) == 0);
}

/* 4. Gửi chuỗi: SendString tự thêm '\n', SendText thì không. */
static void test_uart_send_text(void)
{
    char out[16];
    SendStringInterrupt_UART("abc"); drain(out, sizeof out); CHECK(strcmp(out, "abc\n") == 0);
    SendTextInterrupt_UART("ON");    drain(out, sizeof out); CHECK(strcmp(out, "ON") == 0);
    CHECK(StringEqual("ON", "ON"));
    CHECK(!StringEqual("ON", "OFF"));
}

/* ===================== GPIO ===================== */

/* 1. Cấu hình PB8 output push-pull 2 MHz -> nibble CRH = 0x2, các chân khác giữ nguyên. */
static void test_gpio_config_output(void)
{
    GPIOB->CRH = 0x44444444U;
    GPIO_ConfigPin(GPIOB, 8, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_2MHZ);
    CHECK(GPIOB->CRH == 0x44444442U);
}

/* 2. Output mà truyền tốc độ INPUT là sai -> không đụng vào thanh ghi. */
static void test_gpio_reject_bad_speed(void)
{
    GPIOB->CRH = 0x44444444U;
    GPIO_ConfigPin(GPIOB, 8, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_INPUT);
    CHECK(GPIOB->CRH == 0x44444444U);
}

/* 3. Ghi chân: mức 1 qua BSRR, mức 0 qua BRR (nguyên tử, không đọc-sửa-ghi ODR). */
static void test_gpio_write(void)
{
    GPIO_WritePin(GPIOB, 8, 1); CHECK(GPIOB->BSRR == (1U << 8));
    GPIO_WritePin(GPIOB, 8, 0); CHECK(GPIOB->BRR == (1U << 8));
}

/* ===================== Relay (PA0, kích mức cao) ===================== */

#define RELAY_GPIO  ((GPIO_TypeDef *)BSP_RELAY_GPIO_BASE)
#define BUTTON_GPIO ((GPIO_TypeDef *)BSP_BUTTON_GPIO_BASE)

/* 1. Init: nạp mức OFF (0, qua BRR) trước, pin thành output push-pull 2 MHz (nibble 0x2), trạng thái OFF. */
static void test_relay_init_off(void)
{
    RELAY_GPIO->BSRR = 0; RELAY_GPIO->BRR = 0; RELAY_GPIO->CRL = 0x44444444U;
    relay_init();
    CHECK(RELAY_GPIO->BRR == (1U << BSP_RELAY_PIN) && RELAY_GPIO->BSRR == 0);
    CHECK(((RELAY_GPIO->CRL >> (4U * BSP_RELAY_PIN)) & 0xFU) == 0x2U);
    CHECK(!relay_get());
}

/* 2. Bật: đưa PA0 lên 1 (BSRR), relay_get = true. */
static void test_relay_on(void)
{
    RELAY_GPIO->BSRR = 0; RELAY_GPIO->BRR = 0;
    relay_set(true);
    CHECK(RELAY_GPIO->BSRR == (1U << BSP_RELAY_PIN) && RELAY_GPIO->BRR == 0);
    CHECK(relay_get());
}

/* 3. Tắt: kéo PA0 xuống 0 (BRR), relay_get = false. */
static void test_relay_off(void)
{
    RELAY_GPIO->BSRR = 0; RELAY_GPIO->BRR = 0;
    relay_set(false);
    CHECK(RELAY_GPIO->BRR == (1U << BSP_RELAY_PIN) && RELAY_GPIO->BSRR == 0);
    CHECK(!relay_get());
}

/* ===================== Nút nhấn (PB8, nhấn = 0, lọc 3 mẫu x 20 ms) ===================== */

static void set_button(bool pressed) { BUTTON_GPIO->IDR = pressed ? 0U : (1U << BSP_BUTTON_PIN); }

/* Gọi button_poll_pressed n lần (mỗi lần ứng với 20 ms), đếm số sự kiện nhấn. */
static int poll_events(int n)
{
    int events = 0;
    while (n--) events += button_poll_pressed();
    return events;
}

/* 1. Nhấn một lần, giữ ổn định -> đúng 1 sự kiện. */
static void test_button_press_once(void)
{
    set_button(false); button_init();
    CHECK(poll_events(5) == 0);
    set_button(true);
    CHECK(poll_events(5) == 1);
}

/* 2. Giữ nút lâu rồi thả -> không lặp sự kiện, thả không tạo sự kiện. */
static void test_button_hold_no_repeat(void)
{
    set_button(false); button_init();
    set_button(true);
    CHECK(poll_events(50) == 1);
    set_button(false);
    CHECK(poll_events(5) == 0);
}

/* 3. Dội: mức đổi liên tục mỗi lần đọc -> bị lọc, 0 sự kiện. */
static void test_button_bounce_filtered(void)
{
    set_button(false); button_init();
    int events = 0;
    for (int i = 0; i < 10; i++) { set_button(i & 1); events += button_poll_pressed(); }
    CHECK(events == 0);
}

/* 4. Đang giữ nút lúc cấp nguồn -> không tính; thả ra rồi nhấn lại mới có sự kiện. */
static void test_button_held_at_boot(void)
{
    set_button(true); button_init();
    CHECK(poll_events(10) == 0);
    set_button(false); CHECK(poll_events(5) == 0);
    set_button(true);  CHECK(poll_events(5) == 1);
}

int main(void)
{
    map_region(0x40000000UL, 0x30000U);   /* GPIO, RCC, USART1 */
    map_region(0xE000E000UL, 0x1000U);    /* NVIC */

    test_uart_init();
    test_uart_send_number();
    test_uart_read_line();
    test_uart_send_text();

    test_gpio_config_output();
    test_gpio_reject_bad_speed();
    test_gpio_write();

    test_relay_init_off();
    test_relay_on();
    test_relay_off();

    test_button_press_once();
    test_button_hold_no_repeat();
    test_button_bounce_filtered();
    test_button_held_at_boot();

    printf("HAL host tests: OK (%u checks)\n", checks);
    return 0;
}
