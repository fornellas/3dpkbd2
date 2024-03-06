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

CFLAGS += -D_GNU_SOURCE

# 0x08000000
MAIN_MEMORY_BASE = 134217728
BOOTLOADER_SIZE_KB = 128
FLASH_SIZE_KB = 512

OOCD_INTERFACE = stlink-v2
OOCD_TARGET= stm32f4x

GDB = gdb-multiarch

OPT += \
	-O2 \
	-Wall \
	-Wextra \
	-Wimplicit-function-declaration \
	-Wredundant-decls \
	-Wmissing-prototypes \
	-Wstrict-prototypes \
	-Wundef \
	-Wshadow \
	-Wstrict-prototypes \
	-Werror

include $(OPENCM3_DIR)/mk/genlink-config.mk

# genlink-config.mk does not populate these at first run,
# as it depends on opencm3_stm32f4.a existing.
EXPECTED_LIBNAME = opencm3_stm32f4
ifeq ($(LIBNAME),)
LIBNAME = $(EXPECTED_LIBNAME)
LDLIBS += -l$(LIBNAME)
LIBDEPS += $(OPENCM3_DIR)/lib/lib$(LIBNAME).a
endif
ifneq ($(LIBNAME), $(EXPECTED_LIBNAME))
$(error Unexpected LIBNAME value $(LIBNAME))
endif

include ../rules.mk

$(OPENCM3_DIR)/lib/lib$(LIBNAME).a:
	make -C $(OPENCM3_DIR) lib/stm32/f4

.PHONY: $(OPENCM3_DIR)/lib/lib$(LIBNAME).a

openocd:
	$(Q)openocd --file interface/$(OOCD_INTERFACE).cfg --file target/$(OOCD_TARGET).cfg

.PHONY: openocd

openocd_program: ${PROJECT}.bin $(LDSCRIPT) max_program_size
	$(Q)openocd --file interface/$(OOCD_INTERFACE).cfg --file target/$(OOCD_TARGET).cfg --command "program ${PROJECT}.bin verify reset exit $(MAIN_MEMORY_BASE)"

.PHONY: openocd_program

gdb: $(PROJECT).elf
	$(Q)$(GDB) --command=lib/gdb.init $(PROJECT).elf

.PHONY: gdb

ifeq ($(CUSTOM_LDSCRIPT_TEMPLATE),)
include $(OPENCM3_DIR)/mk/genlink-rules.mk
else
CUSTOM_LDSCRIPT = $(CUSTOM_LDSCRIPT_TEMPLATE:%.template=$(BUILD_DIR)/%)

$(CUSTOM_LDSCRIPT): $(CUSTOM_LDSCRIPT_TEMPLATE)
	@printf "  TEMPLATE\t$<\n"
	$(Q)mkdir -p $(dir $@)
	$(Q)sed -r s/__MAIN_PROGRAM_MEMORY_ROM_ORIGIN__/$(shell dc -e "$(MAIN_MEMORY_BASE) $(BOOTLOADER_SIZE_KB) 1024 * + p")/g < $< | sed -r s/__MAIN_PROGRAM_MEMORY_ROM_LENGTH__/$(shell dc -e "$(FLASH_SIZE_KB) $(BOOTLOADER_SIZE_KB) - p")/g > $@

$(LDSCRIPT): $(CUSTOM_LDSCRIPT) $(OPENCM3_DIR)/lib/lib$(LIBNAME).a
	@printf "  GENLNK  $(DEVICE) ($<)\n"
	$(Q)ln -s -f $< $@

endif

monitor:
	screen /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A700eCbB-if00-port0 115200

.PHONY: monitor
