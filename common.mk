SHARED_DIR = ../common
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=../libopencm3
PROJECT_DEVICES_DATA = ../custom.devices.data


OPENCM3_DEVICES_DATA = ${OPENCM3_DIR}/ld/devices.data
DEVICES_DATA=../generated.devices.data
$(shell cat ${PROJECT_DEVICES_DATA} ${OPENCM3_DEVICES_DATA} > ${DEVICES_DATA})

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk

clean: clean_devices_data

clean_devices_data:
	rm -f ${DEVICES_DATA}

.PHONY: clean_devices_data


dfu: ${PROJECT}.bin
	dfu-util --alt 0 --device 0483:df11 --dfuse-address ${DFUSE_ADDRESS}:leave --download ${PROJECT}.bin

.PHONY: dfu
