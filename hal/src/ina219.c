#include "ina219.h"
#include "board_config.h"
#include "hal_i2c.h"
#include "FreeRTOS.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* TI INA219 register map, big-endian words over I2C. */
#define INA_REG_CONFIG    0x00U
#define INA_REG_BUS       0x02U
#define INA_REG_CURRENT   0x04U
#define INA_REG_CAL       0x05U
#define INA_CONFIG_32V_320MV_12BIT_CONT 0x399FU
#define INA_BUS_CNVR      (1U << 1)
#define INA_BUS_OVF       (1U << 0)
/* Gồm cả thời gian chờ g_i2c_mutex (OLED giữ bus tối đa ~12 ms/page). */
#define INA_I2C_TIMEOUT   pdMS_TO_TICKS(50U)
#define INA_CAL_FACTOR    0.04096f

static float current_lsb_a;
static uint16_t calibration;
static bool initialized;

static hal_status_t read_reg(uint8_t reg, uint16_t *value)
{
    uint8_t bytes[2];
    hal_status_t status = hal_i2c_mem_read(BSP_INA219_ADDR7, reg, bytes, sizeof(bytes),
                                            INA_I2C_TIMEOUT);
    if (status == HAL_OK) *value = (uint16_t)((uint16_t)bytes[0] << 8 | bytes[1]);
    return status;
}

static hal_status_t write_reg(uint8_t reg, uint16_t value)
{
    const uint8_t bytes[2] = {(uint8_t)(value >> 8), (uint8_t)value};
    return hal_i2c_mem_write(BSP_INA219_ADDR7, reg, bytes, sizeof(bytes),
                             INA_I2C_TIMEOUT);
}

hal_status_t ina219_init(float shunt_ohm, float max_current_a)
{
    /* Calibration is limited by register range and ±320mV PGA range.
     * These values must match the actual module shunt and chosen fan current. */
    initialized = false;
    if (!isfinite(shunt_ohm) || !isfinite(max_current_a) ||
        shunt_ohm <= 0.0f || max_current_a <= 0.0f ||
        shunt_ohm * max_current_a > 0.32f) return HAL_INVALID_ARGUMENT;

    /* Choose LSB >= max current/32767 and fit calibration into 15 bits.
     * Rounding calibration downward prevents exceeding representable range. */
    float requested_lsb = max_current_a / 32767.0f;
    float raw_cal = INA_CAL_FACTOR / (requested_lsb * shunt_ohm);
    if (!isfinite(raw_cal) || raw_cal < 1.0f) return HAL_INVALID_ARGUMENT;
    if (raw_cal > 65534.0f) raw_cal = 65534.0f;
    uint16_t cal = (uint16_t)raw_cal & 0xFFFEU; /* CAL[0] always zero */
    if (cal == 0U) return HAL_INVALID_ARGUMENT;
    float actual_lsb = INA_CAL_FACTOR / ((float)cal * shunt_ohm);
    if (actual_lsb * 32767.0f < max_current_a) return HAL_INVALID_ARGUMENT;

    hal_status_t status = hal_i2c_is_ready(BSP_INA219_ADDR7, INA_I2C_TIMEOUT);
    if (status != HAL_OK) return status;
    status = write_reg(INA_REG_CONFIG, INA_CONFIG_32V_320MV_12BIT_CONT);
    if (status != HAL_OK) return status;
    status = write_reg(INA_REG_CAL, cal);
    if (status != HAL_OK) return status;
    uint16_t readback;
    status = read_reg(INA_REG_CONFIG, &readback);
    if (status != HAL_OK) return status;
    if (readback != INA_CONFIG_32V_320MV_12BIT_CONT) return HAL_ERROR;
    status = read_reg(INA_REG_CAL, &readback);
    if (status != HAL_OK) return status;
    if (readback != cal) return HAL_ERROR;
    calibration = cal;
    current_lsb_a = actual_lsb;
    initialized = true;
    return HAL_OK;
}

hal_status_t ina219_read(float *voltage_v, float *current_a)
{
    if (!voltage_v || !current_a) return HAL_INVALID_ARGUMENT;
    if (!initialized) return HAL_NOT_READY;
    uint16_t bus, current, active_cal;
    hal_status_t status = read_reg(INA_REG_BUS, &bus);
    if (status != HAL_OK) return status;
    if (bus & INA_BUS_OVF) return HAL_ERROR;
    if (!(bus & INA_BUS_CNVR)) return HAL_NOT_READY;
    /* A brownout/reset clears calibration. Never report a plausible zero current. */
    status = read_reg(INA_REG_CAL, &active_cal);
    if (status != HAL_OK) return status;
    if (active_cal != calibration) { initialized = false; return HAL_ERROR; }
    status = read_reg(INA_REG_CURRENT, &current);
    if (status != HAL_OK) return status;
    /* BUS bits [15:3] @4mV; CURRENT is signed two's complement. */
    float v = (float)(bus >> 3) * 0.004f;
    float i = (float)(int16_t)current * current_lsb_a;
    *voltage_v = v;
    *current_a = i;
    return HAL_OK;
}