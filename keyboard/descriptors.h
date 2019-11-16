#include <libopencm3/usb/hid.h>
#include <libopencm3/usb/usbstd.h>

extern char usb_serial_number[25];

extern const char *usb_strings[];

extern const struct usb_device_descriptor dev_descr;

#define CONFIGURATION_VALUE 1

extern const struct usb_config_descriptor conf_descr;

extern const struct usb_interface interfaces[];

#define HID_INTERFACE_NUMBER 0

extern const struct usb_interface_descriptor hid_interface;

#define HID_ENDPOINT_NUMBER 1
#define HID_ENDPOINT_IN_ADDR USB_ENDPOINT_ADDR_GEN(USB_ENDPOINT_DIR_IN, HID_ENDPOINT_NUMBER)

extern const struct usb_endpoint_descriptor hid_endpoint;

struct usb_hid_function {
	struct usb_hid_descriptor hid_descriptor;
	struct {
		uint8_t bReportDescriptorType;
		uint16_t wDescriptorLength;
	} __attribute__((packed)) hid_report;
} __attribute__((packed));

extern const struct usb_hid_function hid_function;

extern const uint8_t hid_report_descriptor[64];

// Must match hid_report_descriptor
struct hid_in_report_data {
	uint8_t modifier_keys;
	uint8_t reserved;
	uint8_t keyboard_keys[6];
};

// Must match hid_report_descriptor
typedef uint8_t hid_out_report_data;