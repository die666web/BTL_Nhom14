/* INA219 producer task: only writes the measurement mailbox.
 * Match these two values to the actual shunt and fan before hardware testing. */
#include "task_measure.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "ina219.h"

#define MEASURE_SHUNT_OHM       0.1f  /* R100 = 0.1 ohm; verify module marking */
#define MEASURE_MAX_CURRENT_A   1.0f  /* include startup current; max * R <= 0.32 V */

static TaskHandle_t measure_task_handle;

static void prvMeasureTask(void *argument)
{
    (void)argument;
    const TickType_t period = pdMS_TO_TICKS(APP_MEASURE_PERIOD_MS);
    TickType_t last_wake = xTaskGetTickCount();
    bool sensor_ready = false;
    bool filter_ready = false;
    float filtered_v = 0.0f;
    float filtered_i = 0.0f;

    for (;;) {
        app_measurement_t sample = {0}; /* invalid unless a fresh read succeeds */
        sample.sample_tick = (uint32_t)xTaskGetTickCount();

        if (!sensor_ready) {
            sensor_ready = ina219_init(MEASURE_SHUNT_OHM, MEASURE_MAX_CURRENT_A) == HAL_OK;
        }
        if (sensor_ready) {
            float v, i;
            hal_status_t status = ina219_read(&v, &i);
            if (status == HAL_OK) {
                /* Low-pass filter; first valid sample is used directly. */
                if (!filter_ready) {
                    filtered_v = v;
                    filtered_i = i;
                    filter_ready = true;
                } else {
                    filtered_v += (v - filtered_v) * 0.25f;
                    filtered_i += (i - filtered_i) * 0.25f;
                }
                sample.voltage_v = filtered_v;
                sample.current_a = filtered_i;
                sample.power_w = filtered_v * filtered_i;
                sample.sample_tick = (uint32_t)xTaskGetTickCount();
                sample.valid = true;
            } else {
                filter_ready = false;
                /* CNVR=0 means a conversion has not finished; retry next period.
                 * Other errors may mean reset, NACK, or lost calibration. */
                if (status != HAL_NOT_READY) sensor_ready = false;
            }
        }
        (void)xQueueOverwrite(g_measurement_queue, &sample);
        (void)xTaskDelayUntil(&last_wake, period);
    }
}

BaseType_t task_measure_create(void)
{
    return xTaskCreate(prvMeasureTask, "Measure", APP_MEASURE_STACK,
                       NULL, APP_MEASURE_PRIO, &measure_task_handle);
}