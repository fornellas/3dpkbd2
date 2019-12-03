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

# 0x08000000
MAIN_MEMORY_BASE = 134217728
BOOTLOADER_SIZE_KB = 128
FLASH_SIZE_KB = 512

include $(OPENCM3_DIR)/mk/genlink-config.mk

include ../rules.mk

ifeq ($(CUSTOM_LDSCRIPT_TEMPLATE),)
include $(OPENCM3_DIR)/mk/genlink-rules.mk
else
CUSTOM_LDSCRIPT = $(CUSTOM_LDSCRIPT_TEMPLATE:%.template=$(BUILD_DIR)/%)

$(CUSTOM_LDSCRIPT): $(CUSTOM_LDSCRIPT_TEMPLATE)
	@printf "  TEMPLATE\t$<\n"
	$(Q)mkdir -p $(dir $@)
	$(Q)sed -r s/__MAIN_PROGRAM_MEMORY_ROM_ORIGIN__/$(shell dc -e "$(MAIN_MEMORY_BASE) $(BOOTLOADER_SIZE_KB) 1024 * + p")/g < $< | sed -r s/__MAIN_PROGRAM_MEMORY_ROM_LENGTH__/$(shell dc -e "$(FLASH_SIZE_KB) $(BOOTLOADER_SIZE_KB) - p")/g > $@

$(LDSCRIPT): $(CUSTOM_LDSCRIPT)
	@printf "  GENLNK  $(DEVICE) ($<)\n"
	$(Q)ln -s $< $@

endif