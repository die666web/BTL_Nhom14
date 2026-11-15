#ifndef BOOT_PROTOCOL_H
#define BOOT_PROTOCOL_H
/* Máy trạng thái bootloader theo docs/OTA_CONTRACT.md mục 5:
 *   WAIT_BEGIN -> RECEIVING -> COMPLETE_GRACE -> (chạy app)
 * Không chặn: main gọi boot_protocol_poll() liên tục. */
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BOOT_ST_WAIT_BEGIN = 0,
    BOOT_ST_RECEIVING,
    BOOT_ST_COMPLETE_GRACE,
    BOOT_ST_RUN_APP
} boot_state_t;

#define BOOT_SESSION_TIMEOUT_MS 15000u
#define BOOT_GRACE_MS            2000u

/* Vào WAIT_BEGIN và gửi READY. */
void         boot_protocol_init(void);
void         boot_protocol_poll(void);
boot_state_t boot_protocol_state(void);

#endif
