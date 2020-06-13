#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include <libopencm3/usb/hid.h>
#include <libopencm3/usb/usbstd.h>

//
// Strings
//

extern char usb_serial_number[25];

#define USB_STRINGS_NUM 3
extern const char *usb_strings[USB_STRINGS_NUM];

//
// Device
//

extern const struct usb_device_descriptor dev_descr;

//
// Configuration
//

#define CONFIGURATION_VALUE 1

extern const struct usb_config_descriptor conf_descr;

//
// Interfaces
//

extern const struct usb_interface interfaces[];

#define HID_INTERFACE_NUMBER_BOOT 0
extern const struct usb_interface_descriptor hid_interface_boot;

#define HID_INTERFACE_NUMBER_EXTRA 1
extern const struct usb_interface_descriptor hid_interface_extra;


//
// Endpoint
//

#define HID_ENDPOINT_NUMBER_BOOT 1
#define HID_ENDPOINT_IN_ADDR_BOOT USB_ENDPOINT_ADDR_IN(HID_ENDPOINT_NUMBER_BOOT)
#define HID_ENDPOINT_MAX_PACKET_SIZE_BOOT 8
extern const struct usb_endpoint_descriptor hid_endpoint_boot;

#define HID_ENDPOINT_NUMBER_EXTRA 2
#define HID_ENDPOINT_IN_ADDR_EXTRA USB_ENDPOINT_ADDR_IN(HID_ENDPOINT_NUMBER_EXTRA)
#define HID_ENDPOINT_MAX_PACKET_SIZE_EXTRA 8
extern const struct usb_endpoint_descriptor hid_endpoint_extra;

//
// HID Function
//

struct usb_hid_function {
	struct usb_hid_descriptor hid_descriptor;
	struct {
		uint8_t bReportDescriptorType;
		uint16_t wDescriptorLength;
	} __attribute__((packed)) hid_report;
} __attribute__((packed));

extern const struct usb_hid_function hid_function_boot;
extern const struct usb_hid_function hid_function_extra;

//
// HID Report Descriptor
//

extern const uint8_t hid_report_descriptor_boot[64];
#define KEYBOARD_PAGE_MAX 6
#define KEYBOARD_MODIFIERS_MAX 8
struct hid_in_report_boot_t {
	uint8_t keyboard_keypad_modifiers;
	uint8_t reserved;
	uint8_t keyboard_keypad[KEYBOARD_PAGE_MAX];
} __attribute__((packed));
typedef uint8_t hid_out_report_boot_t;

extern const uint8_t hid_report_descriptor_extra[42];
#define GENERIC_DESKTOP_PAGE_MAX 4
#define CONSUMER_DEVICES_PAGE_MAX 2
struct hid_in_report_extra_t {
	uint8_t generic_desktop[GENERIC_DESKTOP_PAGE_MAX];
	uint16_t consumer_devices[CONSUMER_DEVICES_PAGE_MAX];
} __attribute__((packed));

#define MAX_HID_USAGE_KEYS (KEYBOARD_MODIFIERS_MAX + KEYBOARD_PAGE_MAX + GENERIC_DESKTOP_PAGE_MAX + CONSUMER_DEVICES_PAGE_MAX)

#endif