#include "../common/key.h"
#include "../common/led.h"
#include "hid.h"
#include <libopencm3/usb/hid.h>
#include <stdlib.h>
#include <string.h>

extern uint32_t uptime_ms;
static uint16_t idle_rate_ms = 0;
static uint32_t idle_finish_ms = 0;
static struct hid_in_report_data *last_hid_in_report = NULL;
static struct hid_in_report_data *hid_in_report_buff = NULL;

#define HID_ENDPOINT_NUMBER 1
#define HID_ENDPOINT_IN_ADDR USB_ENDPOINT_ADDR_GEN(USB_ENDPOINT_DIR_IN, HID_ENDPOINT_NUMBER)
#define HID_INTERFACE_NUMBER 0

static struct hid_in_report_data *get_hid_in_report(void);

// Must match hid_in_report_data & hid_out_report_data
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

// Must match hid_report_descriptor
struct hid_in_report_data {
	uint8_t modifier_keys;
	uint8_t reserved;
	uint8_t keyboard_keys[6];
};

// Must match hid_report_descriptor
typedef uint8_t hid_out_report_data;

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

const struct usb_endpoint_descriptor hid_endpoint = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = HID_ENDPOINT_IN_ADDR,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = sizeof(struct hid_in_report_data),
	.bInterval = 0x01,
};

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
	interface_number = req->wIndex;

	if (interface_number != HID_INTERFACE_NUMBER)
		return USBD_REQ_NOTSUPP;

	// 7.1.1 Get_Descriptor Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == USB_REQ_GET_DESCRIPTOR)
	) {
		switch(descriptor_type) {
			case USB_HID_DT_HID:
				*buf = (uint8_t *)&hid_function;
				*len = sizeof(hid_function);
				return USBD_REQ_HANDLED;
			case USB_HID_DT_REPORT:
				*buf = (uint8_t *)&hid_report_descriptor;
				*len = sizeof(hid_report_descriptor);
				return USBD_REQ_HANDLED;
			// case USB_HID_DT_PHYSICAL:
			//  	break;
		}
	}

	// 7.1.2 Set_Descriptor Request
	// if(
	// 	((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
	// 	&& (req->bRequest == USB_REQ_SET_DESCRIPTOR)
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
	uint8_t interface_number;
	uint8_t report_length;

	// TODO
	(void)report_id;
	(void)dev;
	(void)len;
	(void)complete;

	// 7.2.1 Get_Report Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == USB_HID_REQ_TYPE_GET_REPORT)
	) {
		static struct hid_in_report_data *new_hid_in_report;
		static struct hid_in_report_data hid_in_report;

		report_type = req->wValue >> 8;
		// report_id = req->wValue & 0xFF;
		interface_number = req->wIndex;
		report_length = req->wLength;

		if (interface_number != HID_INTERFACE_NUMBER)
			return USBD_REQ_NOTSUPP;

		switch(report_type) {
			case USB_HID_REPORT_TYPE_INPUT:
				new_hid_in_report = get_hid_in_report();
				memcpy(&hid_in_report, new_hid_in_report, sizeof(struct hid_in_report_data));
				free(new_hid_in_report);
				*buf = (uint8_t *)&hid_in_report;
				*len = sizeof(struct hid_in_report_data);
				return USBD_REQ_HANDLED;
			// case USB_HID_REPORT_TYPE_OUTPUT:
			// 	break;
			// case USB_HID_REPORT_TYPE_FEATURE:
			// 	break;
		}
	}

	// 7.2.2 Set_Report Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
		&& (req->bRequest == USB_HID_REQ_TYPE_SET_REPORT)
	) {
		report_type = req->wValue >> 8;
		// report_id = req->wValue & 0xFF;
		interface_number = req->wIndex;
		report_length = req->wLength;

		if (interface_number != HID_INTERFACE_NUMBER)
			return USBD_REQ_NOTSUPP;

		switch(report_type) {
			hid_out_report_data led_report;
			// case USB_HID_REPORT_TYPE_INPUT:
			// 	break;
			case USB_HID_REPORT_TYPE_OUTPUT:
				if (report_length != sizeof(led_report))
					return USBD_REQ_NOTSUPP;

				led_report = *buf[0];

				// TODO OLED Display
				if(led_report & (1<<1)) // Caps Lock
					led_on();
				else
					led_off();
				return USBD_REQ_HANDLED;
			// case USB_HID_REPORT_TYPE_FEATURE:
			// 	break;
		}
	}

	// 7.2.3 Get_Idle Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == USB_HID_REQ_TYPE_SET_IDLE)
	) {
		static uint8_t idle_rate_ms_buff;

		report_id = req->wValue & 0xFF;
		interface_number = req->wIndex;

		if(report_id != 0 || interface_number != HID_INTERFACE_NUMBER)
			return USBD_REQ_NOTSUPP;

		idle_rate_ms_buff = idle_rate_ms / 4;

		*buf = (uint8_t *)&idle_rate_ms_buff;
		*len = sizeof(idle_rate_ms_buff);
	}

	// 7.2.4 Set_Idle Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
		&& (req->bRequest == USB_HID_REQ_TYPE_GET_IDLE)
	) {
		uint16_t duration_ms;
		duration_ms = (uint16_t)(req->wValue >> 8) * 4;
		report_id = req->wValue & 0xFF;
		interface_number = req->wIndex;

		if(report_id != 0 || interface_number != HID_INTERFACE_NUMBER)
			return USBD_REQ_NOTSUPP;

		idle_rate_ms = duration_ms;
		idle_finish_ms = uptime_ms + idle_rate_ms;

		return USBD_REQ_HANDLED;
	}

	// TODO
	// 7.2.5 Get_Protocol Request
	// if(
	// 	((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
	// 	&& (req->bRequest == USB_HID_REQ_TYPE_GET_PROTOCOL)
	// ) {
	// }

	// TODO
	// 7.2.6 Set_Protocol Request
	// if(
	// 	((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
	// 	&& (req->bRequest == USB_HID_REQ_TYPE_SET_PROTOCOL)
	// ) {
	// 	uint8_t protocol;
	// 	protocol = req->wValue;
	// }

	return USBD_REQ_NOTSUPP;
}

static void hid_endpoint_interrupt_in_transfer_complete(usbd_device *usbd_dev, uint8_t ep) {
	(void)usbd_dev;
	(void)ep;
	
	if(last_hid_in_report != NULL)
		free(last_hid_in_report);
	last_hid_in_report = hid_in_report_buff;

	hid_in_report_buff = NULL;
}

void hid_set_config_callback(usbd_device *dev) {
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

   usbd_ep_setup(
           dev,
           HID_ENDPOINT_IN_ADDR,
           USB_ENDPOINT_ATTR_INTERRUPT,
           sizeof(struct hid_in_report_data),
           hid_endpoint_interrupt_in_transfer_complete
   );

	idle_rate_ms = 0;
	idle_finish_ms = 0;
	if(hid_in_report_buff != NULL)
		free(hid_in_report_buff);
	if(last_hid_in_report != NULL)
		free(last_hid_in_report);
	hid_in_report_buff = NULL;
	last_hid_in_report = NULL;
}

static struct hid_in_report_data *get_hid_in_report(void) {
	struct hid_in_report_data *hid_in_report;
	static uint8_t last = 1;

	hid_in_report = calloc(1, sizeof(struct hid_in_report_data));

	// TODO scan keys
	if(key_pressed())
		hid_in_report->keyboard_keys[0] = 4; // A

	return hid_in_report;
}

void hid_poll(usbd_device *dev) {
	struct hid_in_report_data *new_hid_in_report;
	uint32_t now;

	if(hid_in_report_buff != NULL)
		return;

	if(idle_rate_ms == 0) {
		new_hid_in_report = get_hid_in_report();
		if(last_hid_in_report == NULL) {
			hid_in_report_buff = new_hid_in_report;
			usbd_ep_write_packet(dev, HID_ENDPOINT_IN_ADDR, (void *)hid_in_report_buff, sizeof(struct hid_in_report_data));
		} else {
			if(memcmp(new_hid_in_report, last_hid_in_report, sizeof(struct hid_in_report_data))) {
				hid_in_report_buff = new_hid_in_report;
				usbd_ep_write_packet(dev, HID_ENDPOINT_IN_ADDR, (void *)hid_in_report_buff, sizeof(struct hid_in_report_data));
			} else {
				free(new_hid_in_report);
			}
		}
	} else {
		now = uptime_ms;
		new_hid_in_report = get_hid_in_report();
		if(now >= idle_finish_ms) {
			hid_in_report_buff = new_hid_in_report;
			usbd_ep_write_packet(dev, HID_ENDPOINT_IN_ADDR, (void *)hid_in_report_buff, sizeof(struct hid_in_report_data));
			idle_finish_ms = now + idle_rate_ms;
		} else {
			if(last_hid_in_report == NULL) {
				if(memcmp(new_hid_in_report, last_hid_in_report, sizeof(struct hid_in_report_data))) {
					hid_in_report_buff = new_hid_in_report;
					usbd_ep_write_packet(dev, HID_ENDPOINT_IN_ADDR, (void *)hid_in_report_buff, sizeof(struct hid_in_report_data));
				} else {
					free(new_hid_in_report);
				}
			}
		}
	}
}