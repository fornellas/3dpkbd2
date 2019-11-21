PROJECT = keyboard
CUSTOM_LDSCRIPT= $(PROJECT).ld
CFILES = $(wildcard *.c)
CFILES += key.c led.c

UCGLIB_DIR = ucglib
INCLUDES += -I$(UCGLIB_DIR)/csrc
CFILES += $(wildcard $(UCGLIB_DIR)/csrc/*.c)

include ../common.mk