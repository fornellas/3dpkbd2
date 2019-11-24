BUILD_DIR = bin

SHARED_DIR = lib/
CFILES += $(wildcard lib/*.c)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))

DEVICE=stm32f411ceu6
OPENCM3_DIR=../libopencm3

UCGLIB_DIR = ucglib
INCLUDES += -I$(UCGLIB_DIR)/csrc
CFILES += $(wildcard $(UCGLIB_DIR)/csrc/*.c)

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