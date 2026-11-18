CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size
BUILD := build/gcc
TARGET := $(BUILD)/firmware
LDSCRIPT := STM32F103C8Tx_FLASH.ld
FREERTOS := middlewares/FreeRTOS-Kernel
PORT := $(FREERTOS)/portable/GCC/ARM_CM3
# Danh sách source app (phải khớp nhóm file trong keil/MCU_Project.uvprojx).
APP_SRCS := main.c startup.c bsp/bsp.c \
    hal/src/hal_gpio.c hal/src/hal_rcc.c hal/src/hal_uart.c hal/src/hal_i2c.c \
    hal/src/relay.c hal/src/button.c hal/src/ina219.c hal/src/oled.c \
    app/src/app_ipc.c app/src/rtos_hooks.c app/src/task_comm.c app/src/task_control.c \
    app/src/task_display.c app/src/task_measure.c \
    $(FREERTOS)/tasks.c $(FREERTOS)/list.c $(FREERTOS)/queue.c $(FREERTOS)/timers.c \
    $(FREERTOS)/event_groups.c $(FREERTOS)/stream_buffer.c \
    $(PORT)/port.c $(FREERTOS)/portable/MemMang/heap_4.c
OBJS := $(addprefix $(BUILD)/,$(APP_SRCS:.c=.o))
CPPFLAGS := -I. -Ibsp -Ihal/inc -Iapp/inc -I$(FREERTOS)/include -I$(PORT) -DSTM32F10X_MD
CFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -std=c11 -Wall -Wextra -Werror -Og -g3 -ffreestanding -fno-builtin -ffunction-sections -fdata-sections -MMD -MP
LDFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -T$(LDSCRIPT) -nostartfiles --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections,-Map=$(TARGET).map
# App nằm ngay đầu Flash. Không còn bootloader riêng: cập nhật firmware dùng bootloader ROM của ST.
APP_BASE := 0x08000000
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
all: $(TARGET).bin $(TARGET).hex
$(BUILD)/%.o: %.c Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@
# Nạp lần đầu bằng ST-Link. Sau đó cập nhật qua ESP32: upload build/gcc/firmware.bin lên web.
flash: $(TARGET).bin
	st-flash --reset write $(TARGET).bin $(APP_BASE)
# App thử OTA: cùng thư viện với app thật nhưng main chỉ bật LED PC13 + TaskComm (vẫn OTA lại được).
# Upload build/gcc/ota_led_test.bin lên web ESP32. Không thuộc 'all'.
LED_TARGET := $(BUILD)/ota_led_test
LED_OBJS := $(filter-out $(BUILD)/main.o,$(OBJS)) $(BUILD)/tests/ota_led/main_led.o
ota-led: $(LED_TARGET).bin
$(LED_TARGET).elf: $(LED_OBJS) $(LDSCRIPT)
	$(CC) $(LED_OBJS) $(subst $(TARGET).map,$(LED_TARGET).map,$(LDFLAGS)) -o $@
	$(SIZE) $@
$(LED_TARGET).bin: $(LED_TARGET).elf
	$(OBJCOPY) -O binary $< $@
clean:
	rm -rf build/gcc
-include $(OBJS:.o=.d) $(BUILD)/tests/ota_led/main_led.d
.PHONY: all flash ota-led clean
