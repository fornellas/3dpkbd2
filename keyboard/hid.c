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

#define HID_ENDPOINT_ADDR USB_ENDPOINT_CREATE(1, USB_ENDPOINT_DIR_IN)
// TODO validate with interrupt control transfer
#define HID_ENDPOINT_MAX_PACKET_SIZE 8

const struct usb_endpoint_descriptor hid_endpoint = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = HID_ENDPOINT_ADDR,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = HID_ENDPOINT_MAX_PACKET_SIZE,
	// TODO validate what's fastest
	.bInterval = 0x05,
};

#define HID_INTERFACE_NUMBER 0

const struct usb_interface_descriptor hid_iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = HID_INTERFACE_NUMBER,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_HID,
	.bInterfaceSubClass = USB_HID_SUBCLASS_BOOT_INTERFACE,
	.bInterfaceProtocol = USB_HID_PROTOCOL_KEYBOARD,
	.iInterface = 0,

	.endpoint = &hid_endpoint,

	.extra = &hid_function,
	.extralen = sizeof(hid_function),
};

static void hid_endpoint_interrupt_in_callback(usbd_device *usbd_dev, uint8_t ep) {
	// TODO
	(void)usbd_dev;
	(void)ep;
}

static enum usbd_request_return_codes hid_standard_request(
	usbd_device *dev,
	struct usb_setup_data *req,
	uint8_t **buf,
	uint16_t *len,
	void (**complete)(usbd_device *, struct usb_setup_data * )
) {
	uint8_t descriptor_type;
	// uint8_t descriptor_index;
	uint8_t interface_number;

	(void)dev;
	(void)complete;

	descriptor_type = req->wValue >> 8;
	// descriptor_index = req->wValue & 0xFF;

	// Get_Descriptor Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&&
		(req->bRequest == USB_REQ_GET_DESCRIPTOR)
	) {
		switch(descriptor_type) {
			case USB_HID_DT_HID:
				interface_number = req->wIndex;
				if(interface_number == HID_INTERFACE_NUMBER) {
					*buf = (uint8_t *)&hid_function;
					*len = sizeof(hid_function);
					return USBD_REQ_HANDLED;
				}
				break;
			case USB_HID_DT_REPORT:
				*buf = (uint8_t *)&hid_report_descriptor;
				*len = sizeof(hid_report_descriptor);
				return USBD_REQ_HANDLED;
			// case USB_HID_DT_PHYSICAL:
			//  	break;
		}
	}

	// Set_Descriptor Request
	// if(
	// 	((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
	// 	&&
	// 	(req->bRequest == USB_REQ_SET_DESCRIPTOR)
	// ) {
	// }

	return USBD_REQ_NOTSUPP;
}

static enum usbd_request_return_codes hid_class_specific_request(
	usbd_device *dev,
	struct usb_setup_data *req,
	uint8_t **buf,
	uint16_t *len,
	void (**complete)(usbd_device *, struct usb_setup_data * )
) {
	uint8_t report_type;
	uint8_t report_id;

	// 7.2.1 Get_Report Request
	// if(
	// 	((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
	// 	&&
	// 	(req->bRequest == USB_HID_REQ_TYPE_GET_REPORT)
	// ) {
	// 	report_type = req->wValue >> 8;
	// 	report_id = req->wValue & 0xFF;
	// 	// #define USB_HID_REPORT_TYPE_INPUT 1
	// 	// #define USB_HID_REPORT_TYPE_OUTPUT 2
	// 	// #define USB_HID_REPORT_TYPE_FEATURE 3
	//  return USBD_REQ_HANDLED;
	// }

	// 7.2.2 Set_Report Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
		&&
		(req->bRequest == USB_HID_REQ_TYPE_SET_REPORT)
	) {
		uint8_t led_report;

		report_type = req->wValue >> 8;
		report_id = req->wValue & 0xFF;
		switch(report_type) {
			// case USB_HID_REPORT_TYPE_INPUT:
			// 	break;
			case USB_HID_REPORT_TYPE_OUTPUT:
				led_report = *buf[0];
				// Caps Lock
				if(led_report & (1<<1))
					gpio_clear(GPIOC, GPIO13);
				else
					gpio_set(GPIOC, GPIO13);
				break;
			// case USB_HID_REPORT_TYPE_FEATURE:
			// 	break;
		}
		return USBD_REQ_HANDLED;
	}

	// 7.2.3 Get_Idle Request
	// if(
	// 	((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
	// 	&&
	// 	(req->bRequest == USB_HID_REQ_TYPE_SET_IDLE)
	// ) {
	// 	report_id = req->wValue & 0xFF;
	// }

	// 7.2.4 Set_Idle Request
	// if(
	// 	((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
	// 	&&
	// 	(req->bRequest == USB_HID_REQ_TYPE_GET_IDLE)
	// ) {
	// 	uint8_t duration;
	// 	duration = req->wValue >> 8;
	// 	report_id = req->wValue & 0xFF;
	// 	return USBD_REQ_HANDLED;
	// }

	// 7.2.5 Get_Protocol Request
	// if(
	// 	((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
	// 	&&
	// 	(req->bRequest == USB_HID_REQ_TYPE_GET_PROTOCOL)
	// ) {
	// }

	// 7.2.6 Set_Protocol Request
	// if(
	// 	((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
	// 	&&
	// 	(req->bRequest == USB_HID_REQ_TYPE_SET_PROTOCOL)
	// ) {
	// 	uint8_t protocol;
	// 	protocol = req->wValue;
	// }

	return USBD_REQ_NOTSUPP;
}

void hid_set_config_callback(usbd_device *dev)
{
	usbd_ep_setup(
		dev,
		HID_ENDPOINT_ADDR,
		USB_ENDPOINT_ATTR_INTERRUPT,
		HID_ENDPOINT_MAX_PACKET_SIZE,
		hid_endpoint_interrupt_in_callback
	);

	usbd_register_control_callback(
		dev,
		USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
		USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		hid_standard_request
	);

	usbd_register_control_callback(
		dev,
		USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
		USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		hid_class_specific_request
	);
}