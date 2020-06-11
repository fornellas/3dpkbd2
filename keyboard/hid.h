#ifndef HID_H
#define HID_H

#include "descriptors.h"
#include <libopencm3/usb/usbstd.h>
#include <libopencm3/usb/usbd.h>

extern uint8_t hid_protocol;
extern uint16_t hid_idle_rate_ms;
extern hid_out_report_data_boot hid_led_report;

struct hid_usage_t {
	uint16_t page;
	uint16_t id;
} __attribute__((packed));

struct hid_usage_list {
	struct hid_usage_t values[MAX_HID_USAGE_KEYS];
};

void hid_poll(usbd_device *dev);
void hid_set_config_callback(usbd_device *dev);

#endif