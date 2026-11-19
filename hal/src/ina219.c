#include "ina219.h"
#include "board_config.h"
#include "hal_i2c.h"
#include "FreeRTOS.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* Địa chỉ thanh ghi INA219 (mỗi thanh ghi 16 bit, byte cao gửi trước). */
#define INA_REG_CONFIG    0x00U
#define INA_REG_BUS       0x02U
#define INA_REG_CURRENT   0x04U
#define INA_REG_CAL       0x05U

/* Config: dải bus 32 V, dải shunt ±320 mV, ADC 12 bit, đo liên tục. */
#define INA_CONFIG_VALUE  0x399FU

/* Hai bit cờ trong thanh ghi Bus Voltage. */
#define INA_BUS_CNVR      (1U << 1)   /* 1 = đã chuyển đổi xong */
#define INA_BUS_OVF       (1U << 0)   /* 1 = tràn phép tính */

/* Timeout gồm cả thời gian chờ g_i2c_mutex (OLED giữ bus tối đa ~12 ms/page). */
#define INA_I2C_TIMEOUT   pdMS_TO_TICKS(50U)

/* Hằng số trong datasheet: Cal = 0.04096 / (Current_LSB * R_shunt). */
#define INA_CAL_FACTOR    0.04096f

/* Giới hạn điện áp trên shunt ở dải ±320 mV. */
#define INA_MAX_SHUNT_V   0.32f

static float current_lsb_a;     /* Giá trị 1 bit của thanh ghi Current (A) */
static uint16_t calibration;    /* Giá trị đã ghi vào thanh ghi Calibration */
static bool initialized;


/* ---------- Đọc / ghi một thanh ghi 16 bit ---------- */

static hal_status_t read_reg(uint8_t reg, uint16_t *value)
{
    uint8_t bytes[2];

    hal_status_t status = hal_i2c_mem_read(BSP_INA219_ADDR7, reg,
                                           bytes, sizeof(bytes),
                                           INA_I2C_TIMEOUT);

    if (status == HAL_OK)
    {
        *value = (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
    }

    return status;
}

static hal_status_t write_reg(uint8_t reg, uint16_t value)
{
    const uint8_t bytes[2] =
    {
        (uint8_t)(value >> 8),
        (uint8_t)value
    };

    return hal_i2c_mem_write(BSP_INA219_ADDR7, reg,
                             bytes, sizeof(bytes),
                             INA_I2C_TIMEOUT);
}

/* Ghi một thanh ghi rồi đọc lại để chắc chắn INA219 đã nhận đúng. */
static hal_status_t write_and_verify(uint8_t reg, uint16_t value)
{
    hal_status_t status = write_reg(reg, value);

    if (status != HAL_OK)
    {
        return status;
    }

    uint16_t readback;

    status = read_reg(reg, &readback);

    if (status != HAL_OK)
    {
        return status;
    }

    if (readback != value)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}


/* ---------- Tính giá trị Calibration ---------- */

/* Từ shunt và dòng lớn nhất, tính Cal và Current_LSB.
 * Ví dụ R = 0.1 ohm, I_max = 1 A: LSB ~ 30.5 uA, Cal = 13420. */
static hal_status_t compute_calibration(float shunt_ohm, float max_current_a,
                                        uint16_t *out_cal, float *out_lsb)
{
    /* Kiểm tra tham số: phải là số dương, và I_max * R không vượt 320 mV. */
    if (!isfinite(shunt_ohm) || !isfinite(max_current_a))
    {
        return HAL_INVALID_ARGUMENT;
    }

    if (shunt_ohm <= 0.0f || max_current_a <= 0.0f)
    {
        return HAL_INVALID_ARGUMENT;
    }

    if (shunt_ohm * max_current_a > INA_MAX_SHUNT_V)
    {
        return HAL_INVALID_ARGUMENT;
    }

    /* Thanh ghi Current là số có dấu 16 bit: lớn nhất 32767 bước. */
    float requested_lsb = max_current_a / 32767.0f;
    float raw_cal = INA_CAL_FACTOR / (requested_lsb * shunt_ohm);

    if (!isfinite(raw_cal) || raw_cal < 1.0f)
    {
        return HAL_INVALID_ARGUMENT;
    }

    if (raw_cal > 65534.0f)
    {
        raw_cal = 65534.0f;
    }

    /* Làm tròn xuống và xóa bit 0 (datasheet: bit 0 của Cal luôn bằng 0). */
    uint16_t cal = (uint16_t)raw_cal & 0xFFFEU;

    if (cal == 0U)
    {
        return HAL_INVALID_ARGUMENT;
    }

    /* Tính lại LSB thật từ Cal đã làm tròn, rồi kiểm tra vẫn đo đủ I_max. */
    float actual_lsb = INA_CAL_FACTOR / ((float)cal * shunt_ohm);

    if (actual_lsb * 32767.0f < max_current_a)
    {
        return HAL_INVALID_ARGUMENT;
    }

    *out_cal = cal;
    *out_lsb = actual_lsb;

    return HAL_OK;
}


/* ---------- API ---------- */

hal_status_t ina219_init(float shunt_ohm, float max_current_a)
{
    initialized = false;

    /* Bước 1: tính Calibration theo shunt và dòng lớn nhất. */
    uint16_t cal;
    float lsb;

    hal_status_t status = compute_calibration(shunt_ohm, max_current_a, &cal, &lsb);

    if (status != HAL_OK)
    {
        return status;
    }

    /* Bước 2: kiểm tra INA219 có trả lời trên bus I2C không. */
    status = hal_i2c_is_ready(BSP_INA219_ADDR7, INA_I2C_TIMEOUT);

    if (status != HAL_OK)
    {
        return status;
    }

    /* Bước 3: ghi Config và Calibration, đọc lại từng thanh ghi để xác nhận. */
    status = write_and_verify(INA_REG_CONFIG, INA_CONFIG_VALUE);

    if (status != HAL_OK)
    {
        return status;
    }

    status = write_and_verify(INA_REG_CAL, cal);

    if (status != HAL_OK)
    {
        return status;
    }

    calibration = cal;
    current_lsb_a = lsb;
    initialized = true;

    return HAL_OK;
}

hal_status_t ina219_read(float *voltage_v, float *current_a)
{
    if (!voltage_v || !current_a)
    {
        return HAL_INVALID_ARGUMENT;
    }

    if (!initialized)
    {
        return HAL_NOT_READY;
    }

    /* Bước 1: đọc thanh ghi Bus Voltage và kiểm tra hai cờ. */
    uint16_t bus;

    hal_status_t status = read_reg(INA_REG_BUS, &bus);

    if (status != HAL_OK)
    {
        return status;
    }

    if (bus & INA_BUS_OVF)
    {
        return HAL_ERROR;          /* tràn: số đo không dùng được */
    }

    if (!(bus & INA_BUS_CNVR))
    {
        return HAL_NOT_READY;      /* chưa đo xong: chu kỳ sau đọc lại */
    }

    /* Bước 2: đọc lại Calibration. INA219 bị sụt nguồn/reset thì Cal về 0,
     * khi đó dòng đọc ra luôn bằng 0 nhưng trông như hợp lệ -> phải báo lỗi. */
    uint16_t active_cal;

    status = read_reg(INA_REG_CAL, &active_cal);

    if (status != HAL_OK)
    {
        return status;
    }

    if (active_cal != calibration)
    {
        initialized = false;
        return HAL_ERROR;
    }

    /* Bước 3: đọc thanh ghi Current. */
    uint16_t current;

    status = read_reg(INA_REG_CURRENT, &current);

    if (status != HAL_OK)
    {
        return status;
    }

    /* Bước 4: đổi ra đơn vị V và A.
     * Bus: bit 15..3, mỗi bước 4 mV. Current: số có dấu, nhân với LSB. */
    *voltage_v = (float)(bus >> 3) * 0.004f;
    *current_a = (float)(int16_t)current * current_lsb_a;

    return HAL_OK;
}
