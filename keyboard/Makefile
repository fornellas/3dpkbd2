PROJECT = keyboard
CUSTOM_LDSCRIPT= $(PROJECT).ld
CFILES = $(wildcard *.c)
CFILES += key.c led.c display_lib.c

UCGLIB_DIR = ucglib
INCLUDES += -I$(UCGLIB_DIR)/csrc
CFILES += $(wildcard $(UCGLIB_DIR)/csrc/*.c)

include ../common.mk

dfu: ${PROJECT}.bin $(LDSCRIPT)
	@printf "  DFU-UTIL  $<\n"
	$(Q)$(SHARED_DIR)/dfu-util-wrapper.sh --device $(USB_VID):$(USB_PID) --download $(PROJECT).bin --verbose --reset

.PHONY: dfu