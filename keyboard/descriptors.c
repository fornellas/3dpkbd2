#include "descriptors.h"

char usb_serial_number[25];

const char *usb_strings[USB_STRINGS_NUM] = {
	"Fabio Pugliese Ornellas",
	"3D Printed Keyboard 2",
	usb_serial_number,
};

const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = USB_VID,
	.idProduct = USB_PID,
	.bcdDevice = 0x0100,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

const struct usb_config_descriptor conf_descr = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = CONFIGURATION_VALUE,
	.iConfiguration = 0,
	.bmAttributes = (
		USB_CONFIG_ATTR_DEFAULT |
		USB_CONFIG_ATTR_SELF_POWERED |
		USB_CONFIG_ATTR_REMOTE_WAKEUP
	),
	// TODO set accurate value
	.bMaxPower = 250, // 500 mAh

	.interface = interfaces,
};

const struct usb_interface interfaces[] = {
	{
		.num_altsetting = 1,
		.altsetting = &hid_interface,
	}
};

const struct usb_interface_descriptor hid_interface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = HID_INTERFACE_NUMBER,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_HID,
	.bInterfaceSubClass = USB_HID_SUBCLASS_BOOT_INTERFACE,
	.bInterfaceProtocol = USB_HID_INTERFACE_PROTOCOL_KEYBOARD,
	.iInterface = 0,

	.endpoint = &hid_endpoint,

	.extra = &hid_function,
	.extralen = sizeof(hid_function),
};

const struct usb_endpoint_descriptor hid_endpoint = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = HID_ENDPOINT_IN_ADDR,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = sizeof(struct hid_in_report_data),
	.bInterval = 0x01,
};

const struct usb_hid_function hid_function = {
	.hid_descriptor = {
		.bLength = sizeof(hid_function),
		.bDescriptorType = USB_HID_DT_HID,
		.bcdHID = 0x0111,
		.bCountryCode = 0,
		.bNumDescriptors = 1,
	},
	.hid_report = {
		.bReportDescriptorType = USB_HID_DT_REPORT,
		.wDescriptorLength = sizeof(hid_report_descriptor),
	}
};

// Must match hid_in_report_data & hid_out_report_data
const uint8_t hid_report_descriptor[] = {
	// https://www.usb.org/document-library/device-class-definition-hid-111
	// From "Device Class Definition for HID 1.11" Appendix B.
	// This a Boot Interface Descriptor, Protocol 1 (Keyboard) required by
	// BIOS.
	0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
	0x09, 0x06,       // USAGE (Keyboard)
	0xa1, 0x01,       // COLLECTION (Application)
	// Modifier byte
	0x75, 0x01,       //   REPORT_SIZE (1)
	0x95, 0x08,       //   REPORT_COUNT (8)
	0x05, 0x07,       //   USAGE_PAGE (Keyboard)
	0x19, 0xe0,       //   USAGE_MINIMUM (Keyboard LeftControl)
	0x29, 0xe7,       //   USAGE_MAXIMUM (Keyboard Right GUI)
	0x15, 0x00,       //   LOGICAL_MINIMUM (0)
	0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
	0x81, 0x02,       //   INPUT (Data,Var,Abs)
	// Reserved byte
	0x95, 0x01,       //   REPORT_COUNT (1)
	0x75, 0x08,       //   REPORT_SIZE (8)
	0x81, 0x01,       //   INPUT (Cnst)
	// LED report
	0x95, 0x05,       //   REPORT_COUNT (5)
	0x75, 0x01,       //   REPORT_SIZE (1)
	0x05, 0x08,       //   USAGE_PAGE (LEDs)
	0x19, 0x01,       //   USAGE_MINIMUM (Num Lock)
	0x29, 0x05,       //   USAGE_MAXIMUM (Kana)
	0x91, 0x02,       //   OUTPUT (Data,Var,Abs)
	// LED report padding
	0x95, 0x01,       //   REPORT_COUNT (1)
	0x75, 0x03,       //   REPORT_SIZE (3)
	0x91, 0x01,       //   OUTPUT (Cnst)
	// Keyboard/Keypad
	0x95, 0x06,       //   REPORT_COUNT (6)
	0x75, 0x08,       //   REPORT_SIZE (8)
	0x15, 0x00,       //   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00, //   LOGICAL_MAXIMUM (255)
	0x05, 0x07,       //   USAGE_PAGE (Keyboard)
	0x19, 0x00,       //   USAGE_MINIMUM (Reserved (no event indicated))
	0x29, 0xff,       //   USAGE_MAXIMUM 0xFF
	0x81, 0x00,       //   INPUT (Data,Ary)
	// We are allowed to append additional data here, that will not be read
	// by the BIOS.
	// // Generic Desktop
	// 0x95, 0x06,       //   REPORT_COUNT (6)
	// 0x75, 0x08,       //   REPORT_SIZE (8)
	// 0x15, 0x00,       //   LOGICAL_MINIMUM (0)
	// 0x26, 0xff, 0x00, //   LOGICAL_MAXIMUM (255)
	// 0x05, 0x01,       //   USAGE_PAGE (Generic Desktop)
	// 0x19, 0x00,       //   USAGE_MINIMUM Undefined
	// 0x29, 0xff,       //   USAGE_MAXIMUM 0xFF
	// 0x81, 0x00,       //   INPUT (Data,Ary)
	// // Consumer Devices
	// 0x95, 0x06,       //   REPORT_COUNT (6)
	// 0x75, 0x08,       //   REPORT_SIZE (8)
	// 0x15, 0x00,       //   LOGICAL_MINIMUM (0)
	// 0x26, 0x02, 0x9C, //   LOGICAL_MAXIMUM (0x029C)
	// 0x05, 0x0C,       //   USAGE_PAGE (Consumer Devices)
	// 0x19, 0x00,       //   USAGE_MINIMUM Consumer Devices
	// 0x29, 0x02, 0x9C, //   USAGE_MAXIMUM 0x029C
	// 0x81, 0x00,        //   INPUT (Data,Ary)
	0xc0,             // END_COLLECTION
};