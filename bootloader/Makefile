PROJECT = bootloader
CFILES = $(wildcard *.c)
CSTD = -std=gnu99

include ../common.mk

# System DFU Bootloader
# STMicroelectronics / STM32  BOOTLOADER
STM_USB_VID=0x0483
STM_USB_PID=0xdf11

max_program_size: $(PROJECT).bin
	$(Q)test $(shell stat -c %s $<) -lt $(shell dc -e "$(BOOTLOADER_SIZE_KB) 1024 * p") || exit 1

.PHONY: max_program_size

dfu: ${PROJECT}.bin $(LDSCRIPT) max_program_size
	@printf "  DFU-UTIL  $<\n"
	$(Q)$(SHARED_DIR)/dfu-util-wrapper.sh --alt 0 --device $(STM_USB_VID):$(STM_USB_PID) --dfuse-address $(MAIN_MEMORY_BASE) --download $< --reset

.PHONY: dfu