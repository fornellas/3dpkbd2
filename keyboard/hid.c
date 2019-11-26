#include "descriptors.h"
#include "hid.h"
#include "lib/key.h"
#include "lib/led.h"
#include "lib/systick.h"
#include "usb.h"
#include <stdlib.h>
#include <string.h>

//
// Variables
//

uint8_t hid_protocol=1;

static int16_t idle_rate_ms = -1;
static uint32_t idle_finish_ms = 0;
static uint8_t hid_report_transmitting=0;
static struct hid_in_report_data old_hid_in_report;

//
// Prototypes
//

static void get_hid_in_report(struct hid_in_report_data *);

static void send_in_report(usbd_device *dev, struct hid_in_report_data *);

//
// Functions
//

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

	(void)dev;
	(void)complete;

	// 7.2.1 Get_Report Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == USB_HID_REQ_TYPE_GET_REPORT)
	) {
		static struct hid_in_report_data new_hid_in_report;

		report_type = req->wValue >> 8;
		// report_id = req->wValue & 0xFF;
		interface_number = req->wIndex;
		report_length = req->wLength;

		if (interface_number != HID_INTERFACE_NUMBER)
			return USBD_REQ_NOTSUPP;

		switch(report_type) {
			case USB_HID_REPORT_TYPE_INPUT:
				get_hid_in_report(&new_hid_in_report);
				memcpy(*buf, &new_hid_in_report, sizeof(struct hid_in_report_data));
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
		report_id = req->wValue & 0xFF;
		interface_number = req->wIndex;

		if(report_id != 0 || interface_number != HID_INTERFACE_NUMBER)
			return USBD_REQ_NOTSUPP;

		*buf[0] = idle_rate_ms / 4;
		*len = sizeof(uint8_t);
		return USBD_REQ_HANDLED;
	}

	// 7.2.4 Set_Idle Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
		&& (req->bRequest == USB_HID_REQ_TYPE_SET_IDLE)
	) {
		uint16_t duration_ms;
		duration_ms = (uint16_t)(req->wValue >> 8) * 4;
		report_id = req->wValue & 0xFF;
		interface_number = req->wIndex;

		if(report_id != 0 || interface_number != HID_INTERFACE_NUMBER)
			return USBD_REQ_NOTSUPP;

		idle_rate_ms = duration_ms;
		idle_finish_ms = uptime_ms() + idle_rate_ms;

		return USBD_REQ_HANDLED;
	}

	// 7.2.5 Get_Protocol Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == USB_HID_REQ_TYPE_GET_PROTOCOL)
	) {
		interface_number = req->wIndex;

		if (interface_number != HID_INTERFACE_NUMBER)
			return USBD_REQ_NOTSUPP;

		*buf = (uint8_t *)&hid_protocol;
		*len = sizeof(hid_protocol);
		return USBD_REQ_HANDLED;
	}

	// 7.2.6 Set_Protocol Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
		&& (req->bRequest == USB_HID_REQ_TYPE_SET_PROTOCOL)
	) {
		interface_number = req->wIndex;

		if (interface_number != HID_INTERFACE_NUMBER)
			return USBD_REQ_NOTSUPP;

		hid_protocol = req->wValue;

		switch(req->wValue) {
			// Boot Protocol
			case 0:
				// The HID Report matches boot protocol requirements, we can
				// use it for boot protocol as well.
				return USBD_REQ_HANDLED;
			// Report Protocol
			case 1:
				return USBD_REQ_HANDLED;
			default:
				return USBD_REQ_NOTSUPP;
		}
	}

	return USBD_REQ_NOTSUPP;
}

static void hid_endpoint_interrupt_in_transfer_complete(usbd_device *usbd_dev, uint8_t ep) {
	(void)usbd_dev;
	(void)ep;
	
	hid_report_transmitting = 0;
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

	hid_report_transmitting = 0;
	idle_rate_ms = -1;
	idle_finish_ms = 0;

	get_hid_in_report(&old_hid_in_report);
}

static void get_hid_in_report(struct hid_in_report_data *hid_in_report) {
	memset(hid_in_report, 0, sizeof(struct hid_in_report_data));

	// TODO scan keys
	if(key_pressed())
		hid_in_report->keyboard_keys[0] = 4; // A
}


static void send_in_report(usbd_device *dev, struct hid_in_report_data *new_hid_in_report) {
	memcpy(&old_hid_in_report, new_hid_in_report, sizeof(struct hid_in_report_data));
	hid_report_transmitting = 1;
	usbd_ep_write_packet(dev, HID_ENDPOINT_IN_ADDR, (void *)new_hid_in_report, sizeof(struct hid_in_report_data));
}

void hid_poll(usbd_device *dev) {
	struct hid_in_report_data new_hid_in_report;
	uint32_t now;

	if((usbd_state == USBD_STATE_SUSPENDED) && usb_remote_wakeup_enabled) {
		uint8_t *new_hid_in_report_bytes;

		get_hid_in_report(&new_hid_in_report);
		new_hid_in_report_bytes = (uint8_t *)(&new_hid_in_report);

		for(uint16_t i=0 ; i < sizeof(struct hid_in_report_data) ; i++ ) {
			if(new_hid_in_report_bytes[i]) {
				usdb_remote_wakeup_signal();
				usb_remote_wakeup_enabled = 0;
				break;
			}
		}
		return;
	}

	if(!(usbd_state == USBD_STATE_CONFIGURED))
		return;

	if(hid_report_transmitting)
		return;

	// Only send if there are changes
	if(idle_rate_ms == 0) {
		get_hid_in_report(&new_hid_in_report);
		if(memcmp(&new_hid_in_report, &old_hid_in_report, sizeof(struct hid_in_report_data)))
			send_in_report(dev, &new_hid_in_report);
	// Only send if there are changes or at idle rate
	}else if(idle_rate_ms > 0) {
		get_hid_in_report(&new_hid_in_report);
		now = uptime_ms();
		if(now >= idle_finish_ms) {
			send_in_report(dev, &new_hid_in_report);
			idle_finish_ms = now + idle_rate_ms;
		} else {
			if(memcmp(&new_hid_in_report, &old_hid_in_report, sizeof(struct hid_in_report_data)))
				send_in_report(dev, &new_hid_in_report);
		}
	// Always send
	} else {
		get_hid_in_report(&new_hid_in_report);
		send_in_report(dev, &new_hid_in_report);
	}
}