/* TaskControl — Vũ Văn Võ
 * Chu kỳ 20 ms: nút PB8 (chống dội), điều khiển relay PA0,
 * publish app_control_state_t qua g_control_queue (overwrite).
 * OTA ưu tiên tuyệt đối (docs/API_VA_TICH_HOP.md): thấy APP_EVENT_OTA_REQUEST -> khóa, relay OFF, cập nhật mailbox, rồi mới set APP_EVENT_RELAY_OFF_ACK.
 * Khi đã khóa, bỏ qua mọi yêu cầu bật từ nút. Không delay bận, không chờ vô hạn.
 */
#include "task_control.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "button.h"
#include "relay.h"

static bool ota_requested(void)
{
    return (xEventGroupGetBits(g_system_events) & APP_EVENT_OTA_REQUEST) != 0U;
}

static void publish(const app_control_state_t *state)
{
    (void)xQueueOverwrite(g_control_queue, state);
}

static void prvControlTask(void *argument)
{
    (void)argument;
    const TickType_t period = pdMS_TO_TICKS(APP_CONTROL_PERIOD_MS);
    TickType_t last_wake = xTaskGetTickCount();
    app_control_state_t state = { .relay_on = false, .ota_locked = false };

    relay_init();
    button_init();
    publish(&state);

    for (;;) {
        bool want_on = state.relay_on;
        bool lock = state.ota_locked || ota_requested();

        /* Luôn poll nút để bộ chống dội theo kịp, kể cả khi đang khóa. */
        if (button_poll_pressed()) want_on = !want_on;

        /* Kiểm tra lại ngay trước khi ghi relay: Comm có thể vừa đặt REQUEST. */
        if (lock || ota_requested()) {
            lock = true;
            want_on = false;
        }

        if (want_on != state.relay_on || lock != state.ota_locked) {
            relay_set(want_on);
            state.relay_on = relay_get();
            state.ota_locked = lock;
            publish(&state);
        }

        /* ACK chỉ sau khi relay đã OFF và mailbox đã cập nhật. Idempotent mỗi chu kỳ. */
        if (state.ota_locked && !state.relay_on) {
            (void)xEventGroupSetBits(g_system_events, APP_EVENT_RELAY_OFF_ACK);
        }

        (void)xTaskDelayUntil(&last_wake, period);
    }
}

BaseType_t task_control_create(void)
{
    return xTaskCreate(prvControlTask, "Control", APP_CONTROL_STACK,
                       NULL, APP_CONTROL_PRIO, NULL);
}
