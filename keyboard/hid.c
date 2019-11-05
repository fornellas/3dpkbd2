#include "hid.h"
#include <libopencm3/usb/hid.h>
#include <stdlib.h>

static const uint8_t hid_report_descriptor[] = {
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

static const struct {
	struct usb_hid_descriptor hid_descriptor;
	struct {
		uint8_t bReportDescriptorType;
		uint16_t wDescriptorLength;
	} __attribute__((packed)) hid_report;
} __attribute__((packed)) hid_function = {
	.hid_descriptor = {
		.bLength = sizeof(hid_function),
		.bDescriptorType = USB_DT_HID,
		.bcdHID = 0x0111,
		.bCountryCode = 0,
		.bNumDescriptors = 1,
	},
	.hid_report = {
		.bReportDescriptorType = USB_DT_REPORT,
		.wDescriptorLength = sizeof(hid_report_descriptor),
	}
};

#define HID_ENDPOINT_ADDR 0x81

// https://www.beyondlogic.org/usbnutshell/usb5.shtml#EndpointDescriptors
const struct usb_endpoint_descriptor hid_endpoint = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = HID_ENDPOINT_ADDR,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT | USB_ENDPOINT_ATTR_NOSYNC | USB_ENDPOINT_ATTR_DATA,
	.wMaxPacketSize = 8,
	.bInterval = 0x05,
};

// https://www.beyondlogic.org/usbnutshell/usb5.shtml#InterfaceDescriptors
const struct usb_interface_descriptor hid_iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_HID,
	.bInterfaceSubClass = USB_SUBCLASS_HID_BOOT_INTERFACE,
	.bInterfaceProtocol = USB_PROTOCOL_HID_KEYBOARD,
	.iInterface = 5,

	.endpoint = &hid_endpoint,

	.extra = &hid_function,
	.extralen = sizeof(hid_function),
};

static enum usbd_request_return_codes hid_control_request(
	usbd_device *dev,
	struct usb_setup_data *req,
	uint8_t **buf,
	uint16_t *len,
	void (**complete)(usbd_device *, struct usb_setup_data * )
) {
	(void)complete;
	(void)dev;

	if((req->bmRequestType != HID_ENDPOINT_ADDR) ||
	   (req->bRequest != USB_REQ_GET_DESCRIPTOR) ||
	   (req->wValue != 0x2200))
		return USBD_REQ_NOTSUPP;

	/* Handle the HID report descriptor. */
	*buf = (uint8_t *)hid_report_descriptor;
	*len = sizeof(hid_report_descriptor);

	return USBD_REQ_HANDLED;
}

void hid_set_config_callback(usbd_device *dev, uint16_t wValue)
{
	(void)wValue;
	(void)dev;

	usbd_ep_setup(dev, HID_ENDPOINT_ADDR, USB_ENDPOINT_ATTR_INTERRUPT, 4, NULL);

	usbd_register_control_callback(
		dev,
		USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
		USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		hid_control_request
	);
}