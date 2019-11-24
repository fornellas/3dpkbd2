BUILD_DIR = bin

SHARED_DIR = ../common

DEVICE=stm32f411ceu6
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=../libopencm3

 # http://pid.codes/
USB_VID=0x1209
 # TODO register PID
USB_PID=0x3dbd

CFLAGS += -DUSB_VID=$(USB_VID) -DUSB_PID=$(USB_PID)

include $(OPENCM3_DIR)/mk/genlink-config.mk

include ../rules.mk

ifeq ($(CUSTOM_LDSCRIPT),)
include $(OPENCM3_DIR)/mk/genlink-rules.mk
else
$(LDSCRIPT): $(CUSTOM_LDSCRIPT)
	@printf "  GENLNK  $(DEVICE) ($<)\n"
	$(Q)ln -s $< $@
endif