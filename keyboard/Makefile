PROJECT = keyboard
CUSTOM_LDSCRIPT_TEMPLATE= lib/main_program.ld.template
CFILES = $(wildcard *.c)
CFILES += $(wildcard keys/*.c)
CFLAGS += -DUSBD_REMOTE_WAKEUP

include ../common.mk

max_program_size:

dfu: ${PROJECT}.bin $(LDSCRIPT)
	@printf "  DFU-UTIL  $<\n"
	$(Q)$(SHARED_DIR)/dfu-util-wrapper.sh --device $(USB_VID):$(USB_PID) --download $< --verbose --reset

.PHONY: dfu