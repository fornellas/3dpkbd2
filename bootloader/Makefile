PROJECT = bootloader
CFILES = $(wildcard *.c)
CSTD = -std=gnu99

include ../common.mk

# System DFU Bootloader
# STMicroelectronics / STM32  BOOTLOADER
STM_USB_VID=0x0483
STM_USB_PID=0xdf11

MAIN_MEMORY_BASE=0x08000000

dfu: ${PROJECT}.bin $(LDSCRIPT)
	@printf "  DFU-UTIL  $<\n"
	$(Q)$(SHARED_DIR)/dfu-util-wrapper.sh --alt 0 --device $(STM_USB_VID):$(STM_USB_PID) --dfuse-address $(MAIN_MEMORY_BASE):leave --download $(PROJECT).bin

.PHONY: dfu