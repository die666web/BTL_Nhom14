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
CPPFLAGS := -I. -Ibsp -Ihal/inc -Iapp/inc -I$(FREERTOS)/include -I$(PORT) -DSTM32F10X_MD
CFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -std=c11 -Wall -Wextra -Werror -Og -g3 -ffreestanding -fno-builtin -ffunction-sections -fdata-sections -MMD -MP
LDFLAGS := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -T$(LDSCRIPT) -nostartfiles --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections,-Map=$(TARGET).map
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
all: $(TARGET).bin $(TARGET).hex
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
flash: $(TARGET).bin
	st-flash --reset write $< 0x08000000
sync-projects:
	$(PYTHON) tools/sync_projects.py
check-projects:
	$(PYTHON) tools/sync_projects.py --check
clean:
	rm -rf build/gcc
-include $(OBJS:.o=.d)
.PHONY: all flash clean sync-projects check-projects
