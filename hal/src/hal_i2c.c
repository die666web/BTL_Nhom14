/* I2C1 master, polling — Lê Trọng Mạnh
 * PB6 SCL / PB7 SDA, Standard mode 100 kHz, PCLK1 = 8 MHz (board_config.h).
 * Mỗi hàm public = một transaction, tự lấy/nhả g_i2c_mutex; timeout tổng gồm cả chờ mutex.
 * Trình tự đọc theo RM0008 + errata STM32F1 (N=1, N=2 dùng POS, N>=3 dùng BTF).
 * Lỗi bus/timeout: phát tối đa 9 xung SCL + STOP bằng GPIO rồi SWRST khối I2C.
 * Driver thiết bị (ina219.c, oled.c, ...) chỉ gọi API trong hal_i2c.h, không chạm thanh ghi I2C.
 */
#include "hal_i2c.h"
#include "hal_gpio.h"
#include "hal_rcc.h"
#include "board_config.h"
#include "app_ipc.h"
#include "task.h"

typedef struct {
    volatile uint32_t CR1, CR2, OAR1, OAR2, DR, SR1, SR2, CCR, TRISE;
} i2c_regs_t;
#define I2C1 ((i2c_regs_t *)0x40005400UL)

#define CR1_PE     (1U << 0)
#define CR1_START  (1U << 8)
#define CR1_STOP   (1U << 9)
#define CR1_ACK    (1U << 10)
#define CR1_POS    (1U << 11)
#define CR1_SWRST  (1U << 15)
#define SR1_SB     (1U << 0)
#define SR1_ADDR   (1U << 1)
#define SR1_BTF    (1U << 2)
#define SR1_RXNE   (1U << 6)
#define SR1_TXE    (1U << 7)
#define SR1_BERR   (1U << 8)
#define SR1_ARLO   (1U << 9)
#define SR1_AF     (1U << 10)  /* NACK */
#define SR1_ERRORS (SR1_BERR | SR1_ARLO | SR1_AF)
#define SR2_BUSY   (1U << 1)

#define I2C_PCLK1_HZ    BSP_CPU_CLOCK_HZ /* APB1 divider = 1 */
#define I2C_SPEED_HZ    100000UL
#define I2C_FREQ_MHZ    (I2C_PCLK1_HZ / 1000000UL)
/* Giới hạn vòng chờ khi tick không chạy (trong critical, trước scheduler).
 * 1 byte @100 kHz ~ 90 us ~ vài trăm vòng; 5000 vòng ~ vài ms. */
#define I2C_SPIN_LIMIT  5000U
#define I2C_RECOVERY_CLOCKS 9U

#if I2C_FREQ_MHZ < 2UL || I2C_FREQ_MHZ > 36UL
#error "I2C1: PCLK1 phai trong 2..36 MHz"
#endif

static uint8_t initialized;

static TickType_t remaining(TickType_t start, TickType_t budget)
{
    TickType_t elapsed = xTaskGetTickCount() - start;
    return elapsed >= budget ? 0 : budget - elapsed;
}

/* ================= Cấu hình phần cứng ================= */

static void pins_mode(GPIO_Mode mode)
{
    /* GPIO_ConfigPin sửa CRL kiểu read-modify-write: khóa khi đổi lúc runtime. */
    taskENTER_CRITICAL();
    GPIO_ConfigPin(GPIOB, BSP_I2C_SCL_PIN, mode, GPIO_SPEED_2MHZ);
    GPIO_ConfigPin(GPIOB, BSP_I2C_SDA_PIN, mode, GPIO_SPEED_2MHZ);
    taskEXIT_CRITICAL();
}

static void i2c_configure(void)
{
    I2C1->CR1 = CR1_SWRST; /* xóa trạng thái BUSY kẹt (errata analog filter) */
    I2C1->CR1 = 0U;
    I2C1->CR2 = I2C_FREQ_MHZ;
    I2C1->CCR = I2C_PCLK1_HZ / (2UL * I2C_SPEED_HZ); /* Sm, Thigh = Tlow */
    I2C1->TRISE = I2C_FREQ_MHZ + 1U;                /* 1000 ns max rise time */
    I2C1->CR1 = CR1_PE;
}

static void bus_delay(void)
{
    for (volatile uint32_t i = 0; i < 10U; ++i) { } /* ~5 us @ 8 MHz */
}

/* Slave bị reset giữa chừng có thể giữ SDA thấp: phát xung SCL cho tới khi
 * nó nhả SDA, tạo STOP thủ công, rồi trả chân về AF và reset khối I2C. */
static void bus_recover(void)
{
    I2C1->CR1 = 0U;
    GPIO_WritePin(GPIOB, BSP_I2C_SCL_PIN, 1U);
    GPIO_WritePin(GPIOB, BSP_I2C_SDA_PIN, 1U);
    pins_mode(GPIO_MODE_OUTPUT_OD);
    bus_delay();
    for (uint8_t i = 0; i < I2C_RECOVERY_CLOCKS &&
                        !GPIO_ReadPin(GPIOB, BSP_I2C_SDA_PIN); ++i) {
        GPIO_WritePin(GPIOB, BSP_I2C_SCL_PIN, 0U); bus_delay();
        GPIO_WritePin(GPIOB, BSP_I2C_SCL_PIN, 1U); bus_delay();
    }
    GPIO_WritePin(GPIOB, BSP_I2C_SCL_PIN, 0U); bus_delay();
    GPIO_WritePin(GPIOB, BSP_I2C_SDA_PIN, 0U); bus_delay();
    GPIO_WritePin(GPIOB, BSP_I2C_SCL_PIN, 1U); bus_delay();
    GPIO_WritePin(GPIOB, BSP_I2C_SDA_PIN, 1U); bus_delay(); /* STOP */
    pins_mode(GPIO_MODE_AF_OD);
    i2c_configure();
}

/* ================= Chờ cờ ================= */

static hal_status_t wait_sr1(uint32_t mask, TickType_t start, TickType_t timeout)
{
    for (;;) {
        uint32_t sr1 = I2C1->SR1;
        if (sr1 & SR1_ERRORS) return HAL_ERROR;
        if (sr1 & mask) return HAL_OK;
        if (remaining(start, timeout) == 0U) return HAL_TIMEOUT;
    }
}

/* Bản dùng trong critical section: tick không tăng nên giới hạn bằng số vòng. */
static hal_status_t spin_sr1(uint32_t mask)
{
    for (uint32_t n = 0; n < I2C_SPIN_LIMIT; ++n) {
        uint32_t sr1 = I2C1->SR1;
        if (sr1 & SR1_ERRORS) return HAL_ERROR;
        if (sr1 & mask) return HAL_OK;
    }
    return HAL_TIMEOUT;
}

/* Bus rảnh: không BUSY và STOP/START trước đã phát xong. */
static hal_status_t spin_idle(void)
{
    for (uint32_t n = 0; n < I2C_SPIN_LIMIT; ++n) {
        if (!(I2C1->SR2 & SR2_BUSY) && !(I2C1->CR1 & (CR1_START | CR1_STOP)))
            return HAL_OK;
    }
    return HAL_TIMEOUT;
}

static void clear_addr(void) { (void)I2C1->SR1; (void)I2C1->SR2; }

/* ================= Khung một transaction ================= */

/* Trả HAL_OK nghĩa là đang giữ mutex; mọi nhánh sau đó phải đi qua end(). */
static hal_status_t begin(TickType_t timeout, TickType_t *start)
{
    if (timeout == portMAX_DELAY) return HAL_INVALID_ARGUMENT;
    if (!initialized || xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
        return HAL_NOT_READY;
    *start = xTaskGetTickCount();
    if (xSemaphoreTake(g_i2c_mutex, timeout) != pdPASS) return HAL_TIMEOUT;
    if (spin_idle() != HAL_OK) {
        bus_recover();
        if (spin_idle() != HAL_OK) {
            (void)xSemaphoreGive(g_i2c_mutex);
            return HAL_ERROR;
        }
    }
    return HAL_OK;
}

static hal_status_t end(hal_status_t status)
{
    if (status != HAL_OK) {
        uint32_t sr1 = I2C1->SR1;
        I2C1->CR1 |= CR1_STOP;
        I2C1->SR1 = 0U; /* bit lỗi là rc_w0 */
        if (status == HAL_TIMEOUT || (sr1 & (SR1_BERR | SR1_ARLO))) bus_recover();
    }
    I2C1->CR1 &= ~(CR1_ACK | CR1_POS);
    (void)spin_idle(); /* STOP ra dây xong mới nhường bus */
    (void)xSemaphoreGive(g_i2c_mutex);
    return status;
}

static hal_status_t send_address(uint8_t addr_rw, TickType_t start, TickType_t timeout)
{
    I2C1->CR1 |= CR1_START;
    hal_status_t status = wait_sr1(SR1_SB, start, timeout);
    if (status != HAL_OK) return status;
    I2C1->DR = addr_rw; /* đọc SR1 rồi ghi DR: xóa SB */
    return wait_sr1(SR1_ADDR, start, timeout);
}

/* (Repeated) START hướng đọc. ACK bật trước để byte đầu được ACK (cần cho N=2/POS);
 * read_bytes() tự tắt ACK đúng thời điểm. */
static hal_status_t read_start(uint8_t addr7, TickType_t start, TickType_t timeout)
{
    I2C1->CR1 |= CR1_ACK;
    return send_address((uint8_t)((addr7 << 1) | 1U), start, timeout);
}

/* Gọi sau khi ADDR đã xóa, hướng ghi. Không phát STOP. */
static hal_status_t send_bytes(const uint8_t *data, size_t length,
                               TickType_t start, TickType_t timeout)
{
    for (size_t i = 0; i < length; ++i) {
        hal_status_t status = wait_sr1(SR1_TXE, start, timeout);
        if (status != HAL_OK) return status;
        I2C1->DR = data[i];
    }
    return HAL_OK;
}

/* Gọi khi ADDR=1 (chưa xóa), hướng đọc. Tự phát STOP đúng thời điểm. */
static hal_status_t read_bytes(uint8_t *data, size_t length,
                               TickType_t start, TickType_t timeout)
{
    hal_status_t status = HAL_OK;
    if (length == 1U) {
        I2C1->CR1 &= ~CR1_ACK;
        taskENTER_CRITICAL();
        clear_addr();
        I2C1->CR1 |= CR1_STOP;
        taskEXIT_CRITICAL();
    } else if (length == 2U) {
        I2C1->CR1 |= CR1_POS;
        taskENTER_CRITICAL();
        clear_addr();
        I2C1->CR1 &= ~CR1_ACK;
        taskEXIT_CRITICAL();
    } else {
        clear_addr(); /* ACK=1 từ read_start */
    }

    while (length > 0U) {
        if (length == 1U || length > 3U) {
            if ((status = wait_sr1(SR1_RXNE, start, timeout)) != HAL_OK) return status;
            *data++ = (uint8_t)I2C1->DR;
            --length;
        } else if (length == 2U) {
            if ((status = wait_sr1(SR1_BTF, start, timeout)) != HAL_OK) return status;
            taskENTER_CRITICAL();
            I2C1->CR1 |= CR1_STOP;
            *data++ = (uint8_t)I2C1->DR;
            taskEXIT_CRITICAL();
            *data = (uint8_t)I2C1->DR;
            length = 0U;
        } else { /* còn 3 byte: NACK byte cuối theo EV7_2 */
            if ((status = wait_sr1(SR1_BTF, start, timeout)) != HAL_OK) return status;
            I2C1->CR1 &= ~CR1_ACK;
            taskENTER_CRITICAL();
            *data++ = (uint8_t)I2C1->DR;
            status = spin_sr1(SR1_BTF);
            if (status == HAL_OK) {
                I2C1->CR1 |= CR1_STOP;
                *data++ = (uint8_t)I2C1->DR;
            }
            taskEXIT_CRITICAL();
            if (status != HAL_OK) return status;
            *data = (uint8_t)I2C1->DR;
            length = 0U;
        }
    }
    return HAL_OK;
}

/* Ghi: [prefix nếu có] + data, kết thúc bằng STOP. Dùng chung cho write/mem_write/is_ready. */
static hal_status_t write_transfer(uint8_t addr7, const uint8_t *prefix,
                                   const uint8_t *data, size_t length, TickType_t timeout)
{
    TickType_t start;
    hal_status_t status = begin(timeout, &start);
    if (status != HAL_OK) return status;

    status = send_address((uint8_t)(addr7 << 1), start, timeout);
    if (status == HAL_OK) {
        clear_addr();
        if (prefix) status = send_bytes(prefix, 1U, start, timeout);
        if (status == HAL_OK) status = send_bytes(data, length, start, timeout);
        if (status == HAL_OK && (prefix || length)) status = wait_sr1(SR1_BTF, start, timeout);
        if (status == HAL_OK) I2C1->CR1 |= CR1_STOP;
    }
    return end(status);
}

/* ================= API ================= */

hal_status_t hal_i2c_init(void)
{
    if (initialized) return HAL_ERROR;          /* init once */
    if (!g_i2c_mutex) return HAL_NOT_READY;     /* app_ipc_init() trước */
    RCC_EnableGPIOBClock();
    RCC_EnableAPB1Clock(RCC_APB1_I2C1);
    bus_recover(); /* giải phóng slave đang kẹt sau khi MCU reset, rồi cấu hình I2C */
    if (spin_idle() != HAL_OK) return HAL_ERROR; /* SDA/SCL bị kéo thấp: kiểm tra pull-up */
    initialized = 1U;
    return HAL_OK;
}

hal_status_t hal_i2c_is_ready(uint8_t addr7, TickType_t timeout)
{
    if (addr7 > 0x7FU) return HAL_INVALID_ARGUMENT;
    return write_transfer(addr7, NULL, NULL, 0U, timeout);
}

hal_status_t hal_i2c_write(uint8_t addr7, const uint8_t *data, size_t length, TickType_t timeout)
{
    if (addr7 > 0x7FU || !data || length == 0U) return HAL_INVALID_ARGUMENT;
    return write_transfer(addr7, NULL, data, length, timeout);
}

hal_status_t hal_i2c_mem_write(uint8_t addr7, uint8_t reg, const uint8_t *data, size_t length, TickType_t timeout)
{
    if (addr7 > 0x7FU || (!data && length)) return HAL_INVALID_ARGUMENT;
    return write_transfer(addr7, &reg, data, length, timeout);
}

hal_status_t hal_i2c_read(uint8_t addr7, uint8_t *data, size_t length, TickType_t timeout)
{
    TickType_t start;
    if (addr7 > 0x7FU || !data || length == 0U) return HAL_INVALID_ARGUMENT;
    hal_status_t status = begin(timeout, &start);
    if (status != HAL_OK) return status;

    status = read_start(addr7, start, timeout);
    if (status == HAL_OK) status = read_bytes(data, length, start, timeout);
    return end(status);
}

hal_status_t hal_i2c_mem_read(uint8_t addr7, uint8_t reg, uint8_t *data, size_t length, TickType_t timeout)
{
    TickType_t start;
    if (addr7 > 0x7FU || !data || length == 0U) return HAL_INVALID_ARGUMENT;
    hal_status_t status = begin(timeout, &start);
    if (status != HAL_OK) return status;

    status = send_address((uint8_t)(addr7 << 1), start, timeout);
    if (status == HAL_OK) {
        clear_addr();
        status = send_bytes(&reg, 1U, start, timeout);
    }
    if (status == HAL_OK) status = wait_sr1(SR1_BTF, start, timeout); /* reg ra dây hết rồi mới repeated START */
    if (status == HAL_OK) status = read_start(addr7, start, timeout);
    if (status == HAL_OK) status = read_bytes(data, length, start, timeout);
    return end(status);
}
