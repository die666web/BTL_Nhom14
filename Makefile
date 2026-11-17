CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size
PYTHON ?= python3
BUILD := build/gcc
TARGET := $(BUILD)/firmware
LDSCRIPT := STM32F103C8Tx_FLASH.ld
FREERTOS := middlewares/FreeRTOS-Kernel
PORT := $(FREERTOS)/portable/GCC/ARM_CM3
include tools/sources.mk
OBJS := $(addprefix $(BUILD)/,$(APP_SRCS:.c=.o))
CPPFLAGS := -I. -Ibsp -Ihal/inc -Iapp/inc -Icommon/ota_frame -I$(FREERTOS)/include -I$(PORT) -DSTM32F10X_MD
CFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -std=c11 -Wall -Wextra -Werror -Og -g3 -ffreestanding -fno-builtin -ffunction-sections -fdata-sections -MMD -MP
LDFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -T$(LDSCRIPT) -nostartfiles --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections,-Map=$(TARGET).map
# Bootloader: target riêng ở 0x08000000 (8 KiB), không FreeRTOS. App ở 0x08002000 (common/ota_frame/ota_layout.h).
APP_BASE := 0x08002000
BOOT_BUILD := build/gcc-boot
BOOT_TARGET := $(BOOT_BUILD)/bootloader
BOOT_LDSCRIPT := bootloader/STM32F103C8_BOOT.ld
BOOT_OBJS := $(addprefix $(BOOT_BUILD)/,$(BOOT_SRCS:.c=.o))
BOOT_CPPFLAGS := -Ibootloader/inc -Ibsp -Icommon/ota_frame -DSTM32F10X_MD
BOOT_CFLAGS := $(filter-out -Og,$(CFLAGS)) -Os
BOOT_LDFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -T$(BOOT_LDSCRIPT) -nostartfiles --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections,-Map=$(BOOT_TARGET).map
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
all: $(TARGET).bin $(TARGET).hex boot
boot: $(BOOT_TARGET).bin $(BOOT_TARGET).hex
$(BOOT_BUILD)/%.o: %.c Makefile tools/sources.mk
	@mkdir -p $(dir $@)
	$(CC) $(BOOT_CPPFLAGS) $(BOOT_CFLAGS) -c $< -o $@
$(BOOT_TARGET).elf: $(BOOT_OBJS) $(BOOT_LDSCRIPT)
	$(CC) $(BOOT_OBJS) $(BOOT_LDFLAGS) -o $@
	$(SIZE) $@
$(BUILD)/%.o: %.c Makefile tools/sources.mk
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@
$(BOOT_TARGET).bin: $(BOOT_TARGET).elf
	$(OBJCOPY) -O binary $< $@
$(BOOT_TARGET).hex: $(BOOT_TARGET).elf
	$(OBJCOPY) -O ihex $< $@
# Nạp lần đầu bằng ST-Link: flash-boot (0x08000000) rồi flash (app 0x08002000 + metadata 0x0800FC00).
# Sau đó cập nhật app qua ESP32: upload build/gcc/firmware.bin lên web.
flash: $(TARGET).bin $(BUILD)/meta.bin
	st-flash write $(TARGET).bin $(APP_BASE)
	st-flash --reset write $(BUILD)/meta.bin 0x0800FC00
$(BUILD)/meta.bin: $(TARGET).bin tools/make_meta.py
	$(PYTHON) tools/make_meta.py $< $@
flash-boot: $(BOOT_TARGET).bin
	st-flash --reset write $< 0x08000000
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
sync-projects:
	$(PYTHON) tools/sync_projects.py
check-projects:
	$(PYTHON) tools/sync_projects.py --check
clean:
	rm -rf build/gcc build/gcc-boot
-include $(OBJS:.o=.d) $(BOOT_OBJS:.o=.d) $(BUILD)/tests/ota_led/main_led.d
.PHONY: all boot flash flash-boot ota-led clean sync-projects check-projects
