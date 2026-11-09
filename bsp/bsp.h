#ifndef BSP_H
#define BSP_H
void bsp_system_init(void);
void bsp_relay_force_off(void);
void bsp_panic(void) __attribute__((noreturn));
#endif
