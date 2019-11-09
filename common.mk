BUILD_DIR = bin

SHARED_DIR = ../common

DEVICE=stm32f411ceu6
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk

include ../rules.mk

ifeq ($(CUSTOM_LDSCRIPT),)
include $(OPENCM3_DIR)/mk/genlink-rules.mk
else
$(LDSCRIPT): $(CUSTOM_LDSCRIPT)
	@printf "  GENLNK  $(DEVICE) ($<)\n"
	$(Q)ln -s $< $@
endif

dfu: ${PROJECT}.bin $(LDSCRIPT)
	@printf "  DFU-UTIL  $<\n"
	$(Q)$(SHARED_DIR)/dfu-util-wrapper.sh $(LDSCRIPT) $(PROJECT).bin

.PHONY: dfu