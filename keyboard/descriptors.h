#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include <libopencm3/usb/hid.h>
#include <libopencm3/usb/usbstd.h>

extern char usb_serial_number[25];

#define USB_STRINGS_NUM 3
extern const char *usb_strings[USB_STRINGS_NUM];

extern const struct usb_device_descriptor dev_descr;

#define CONFIGURATION_VALUE 1

extern const struct usb_config_descriptor conf_descr;

extern const struct usb_interface interfaces[];

#define HID_INTERFACE_NUMBER 0
#define HID_INTERFACE_NUMBER_SECONDARY 1

extern const struct usb_interface_descriptor hid_interface;
extern const struct usb_interface_descriptor hid_interface_secondary;

#define HID_ENDPOINT_NUMBER 1
#define HID_ENDPOINT_IN_ADDR USB_ENDPOINT_ADDR_IN(HID_ENDPOINT_NUMBER)
#define HID_ENDPOINT_MAX_PACKET_SIZE 8

#define HID_ENDPOINT_NUMBER_SECONDARY 2
#define HID_ENDPOINT_IN_ADDR_SECONDARY USB_ENDPOINT_ADDR_IN(HID_ENDPOINT_NUMBER_SECONDARY)
#define HID_ENDPOINT_SECONDARY_MAX_PACKET_SIZE 8

extern const struct usb_endpoint_descriptor hid_endpoint;
extern const struct usb_endpoint_descriptor hid_endpoint_secondary;

struct usb_hid_function {
	struct usb_hid_descriptor hid_descriptor;
	struct {
		uint8_t bReportDescriptorType;
		uint16_t wDescriptorLength;
	} __attribute__((packed)) hid_report;
} __attribute__((packed));

extern const struct usb_hid_function hid_function;
extern const struct usb_hid_function hid_function_secondary;

extern const uint8_t hid_report_descriptor[64];
extern const uint8_t hid_report_descriptor_secondary[42];

#define KEYBOARD_PAGE_MAX 6

#define HID_IN_REPORT_DATA_MAX_KEYBOARD_KEYPAD 6
#define HID_IN_REPORT_DATA_MAX_GENERIC_DESKTOP 4
#define HID_IN_REPORT_DATA_MAX_CONSUMER_DEVICES 2

// Must match hid_report_descriptor, HID_ENDPOINT_MAX_PACKET_SIZE
struct hid_in_report_data {
	uint8_t keyboard_keypad_modifiers;
	uint8_t reserved;
	uint8_t keyboard_keypad[KEYBOARD_PAGE_MAX];
} __attribute__((packed));

#define GENERIC_DESKTOP_PAGE_MAX 4
#define CONSUMER_DEVICES_PAGE_MAX 2

// Must match hid_report_descriptor_secondary
struct hid_in_report_data_secondary {
	uint8_t generic_desktop[GENERIC_DESKTOP_PAGE_MAX];
	uint16_t consumer_devices[CONSUMER_DEVICES_PAGE_MAX];
} __attribute__((packed));

// Must match hid_report_descriptor
void hid_in_report_add(struct hid_in_report_data *, uint16_t, uint16_t);

// Must match hid_report_descriptor
typedef uint8_t hid_out_report_data;

#endif