PROJECT = keyboard
CUSTOM_LDSCRIPT= $(PROJECT).ld
CFILES = $(wildcard *.c)

include ../common.mk

dfu: ${PROJECT}.bin $(LDSCRIPT)
	@printf "  DFU-UTIL  $<\n"
	$(Q)$(SHARED_DIR)/dfu-util-wrapper.sh --device $(USB_VID):$(USB_PID) --download $(PROJECT).bin --verbose --reset

.PHONY: dfu