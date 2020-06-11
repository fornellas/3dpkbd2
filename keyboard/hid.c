#include "descriptors.h"
#include "hid.h"
#include "lib/systick.h"
#include "usb.h"
#include <stdlib.h>
#include <string.h>
#include "keys.h"

//
// Variables
//

uint8_t hid_protocol = USB_HID_PROTOCOL_REPORT;
uint16_t hid_idle_rate_ms = 0;
hid_out_report_data_boot hid_led_report;

static uint32_t idle_finish_ms = 0;
static uint8_t hid_report_transmitting_boot = 0;
static uint8_t hid_report_transmitting_extra = 0;
static struct hid_in_report_data_boot old_hid_in_report;
static uint8_t hid_usbd_remote_wakeup_sent = 0;

//
// Prototypes
//

static void get_hid_in_report(struct hid_in_report_data_boot *);

static void send_in_report(usbd_device *dev, struct hid_in_report_data_boot *);

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

	if (interface_number != HID_INTERFACE_NUMBER_BOOT && interface_number != HID_INTERFACE_NUMBER_EXTRA)
		return USBD_REQ_NOTSUPP;

	// 7.1.1 Get_Descriptor Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == USB_REQ_GET_DESCRIPTOR)
	) {
		if(interface_number == HID_INTERFACE_NUMBER_BOOT) {
			switch(descriptor_type) {
				case USB_HID_DT_HID:
					*buf = (uint8_t *)&hid_function_boot;
					*len = sizeof(hid_function_boot);
					return USBD_REQ_HANDLED;
				case USB_HID_DT_REPORT:
					*buf = (uint8_t *)&hid_report_descriptor_boot;
					*len = sizeof(hid_report_descriptor_boot);
					return USBD_REQ_HANDLED;
				// case USB_HID_DT_PHYSICAL:
				//  	break;
			}
		} else if(interface_number == HID_INTERFACE_NUMBER_EXTRA) {
			switch(descriptor_type) {
				case USB_HID_DT_HID:
					*buf = (uint8_t *)&hid_function_extra;
					*len = sizeof(hid_function_extra);
					return USBD_REQ_HANDLED;
				case USB_HID_DT_REPORT:
					*buf = (uint8_t *)&hid_report_descriptor_extra;
					*len = sizeof(hid_report_descriptor_extra);
					return USBD_REQ_HANDLED;
				// case USB_HID_DT_PHYSICAL:
				//  	break;
			}
		} else
			return USBD_REQ_NOTSUPP;
	}

	// 7.1.2 Set_Descriptor Request
	// if(
	// 	((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
	// 	&& (req->bRequest == USB_REQ_SET_DESCRIPTOR)
	// ) {
	// }

	return USBD_REQ_NEXT_CALLBACK;
}

static enum usbd_request_return_codes hid_class_get_report(
	uint8_t report_type,
	uint8_t report_id,
	uint8_t interface_number,
	uint8_t report_length,
	uint8_t **buf,
	uint16_t *len
) {
	static struct hid_in_report_data_boot new_hid_in_report;
	(void)report_id;
	(void)report_length;

	if (interface_number != HID_INTERFACE_NUMBER_BOOT)
		return USBD_REQ_NOTSUPP;

	switch(report_type) {
		case USB_HID_REPORT_TYPE_INPUT:
			get_hid_in_report(&new_hid_in_report);
			memcpy(*buf, &new_hid_in_report, sizeof(struct hid_in_report_data_boot));
			// For Boot Protocol we can only send the first 8 bytes
			if(!hid_protocol)
				*len = 8;
			else
				*len = sizeof(struct hid_in_report_data_boot);
			return USBD_REQ_HANDLED;
		// case USB_HID_REPORT_TYPE_OUTPUT:
		// 	break;
		// case USB_HID_REPORT_TYPE_FEATURE:
		// 	break;
	}
	return USBD_REQ_NOTSUPP;
}

static enum usbd_request_return_codes hid_class_set_report(
	uint8_t report_type,
	uint8_t report_id,
	uint8_t interface_number,
	uint8_t report_length,
	uint8_t **buf,
	uint16_t *len
) {
	(void)report_id;
	(void)len;

	if (interface_number != HID_INTERFACE_NUMBER_BOOT)
		return USBD_REQ_NOTSUPP;

	switch(report_type) {
		// case USB_HID_REPORT_TYPE_INPUT:
		// 	break;
		case USB_HID_REPORT_TYPE_OUTPUT:
			if (report_length != sizeof(hid_led_report))
				return USBD_REQ_NOTSUPP;
			// FIXME check len
			hid_led_report = *buf[0];
			return USBD_REQ_HANDLED;
		// case USB_HID_REPORT_TYPE_FEATURE:
		// 	break;
	}
	return USBD_REQ_NOTSUPP;
}

static enum usbd_request_return_codes hid_class_get_idle(
	uint8_t report_id,
	uint8_t interface_number,
	uint8_t **buf,
	uint16_t *len
) {
	if(report_id != 0 || interface_number != HID_INTERFACE_NUMBER_BOOT)
		return USBD_REQ_NOTSUPP;

	*buf[0] = hid_idle_rate_ms / 4;
	*len = sizeof(uint8_t);
	return USBD_REQ_HANDLED;
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

	interface_number = req->wIndex;

	// 7.2.1 Get_Report Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == USB_HID_REQ_TYPE_GET_REPORT)
	) {
		return hid_class_get_report(
			req->wValue >> 8,
			req->wValue & 0xFF,
			interface_number,
			req->wLength,
			buf,
			len
		);
	}

	// 7.2.2 Set_Report Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
		&& (req->bRequest == USB_HID_REQ_TYPE_SET_REPORT)
	) {
		return hid_class_set_report(
			req->wValue >> 8,
			req->wValue & 0xFF,
			interface_number,
			req->wLength,
			buf,
			len
		);
	}

	// 7.2.3 Get_Idle Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == USB_HID_REQ_TYPE_GET_IDLE)
	) {
		return hid_class_get_idle(
			req->wValue & 0xFF,
			interface_number,
			buf,
			len
		);
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

		if(report_id != 0 || interface_number != HID_INTERFACE_NUMBER_BOOT)
			return USBD_REQ_NOTSUPP;

		hid_idle_rate_ms = duration_ms;
		idle_finish_ms = uptime_ms() + hid_idle_rate_ms;

		return USBD_REQ_HANDLED;
	}

	// 7.2.5 Get_Protocol Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == USB_HID_REQ_TYPE_GET_PROTOCOL)
	) {
		interface_number = req->wIndex;

		if (interface_number != HID_ENDPOINT_NUMBER_BOOT)
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

		if (interface_number != HID_ENDPOINT_NUMBER_BOOT)
			return USBD_REQ_NOTSUPP;

		hid_protocol = req->wValue;

		switch(hid_protocol) {
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

	return USBD_REQ_NEXT_CALLBACK;
}

static void hid_endpoint_interrupt_in_transfer_complete_boot(usbd_device *usbd_dev, uint8_t ep) {
	(void)usbd_dev;
	(void)ep;
	
	hid_report_transmitting_boot = 0;
}

static void hid_endpoint_interrupt_in_transfer_complete_extra(usbd_device *usbd_dev, uint8_t ep) {
	(void)usbd_dev;
	(void)ep;
	
	hid_report_transmitting_extra = 0;
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
		HID_ENDPOINT_IN_ADDR_BOOT,
		USB_ENDPOINT_ATTR_INTERRUPT,
		HID_ENDPOINT_MAX_PACKET_SIZE_BOOT,
		hid_endpoint_interrupt_in_transfer_complete_boot
	);

   usbd_ep_setup(
		dev,
		HID_ENDPOINT_IN_ADDR_EXTRA,
		USB_ENDPOINT_ATTR_INTERRUPT,
		HID_ENDPOINT_MAX_PACKET_SIZE_EXTRA,
		hid_endpoint_interrupt_in_transfer_complete_extra
	);

	hid_protocol = USB_HID_PROTOCOL_REPORT;
	hid_idle_rate_ms = 0;
	idle_finish_ms = 0;
	hid_report_transmitting_boot = 0;
	hid_report_transmitting_extra = 0;
	keys_reset();
	get_hid_in_report(&old_hid_in_report);
	hid_usbd_remote_wakeup_sent = 0;
}

static void get_hid_in_report(struct hid_in_report_data_boot *hid_in_report) {
	memset(hid_in_report, 0, sizeof(struct hid_in_report_data_boot));
	keys_populate_hid_in_report(hid_in_report);
}

static void send_in_report(usbd_device *dev, struct hid_in_report_data_boot *new_hid_in_report) {
	uint16_t len;

	memcpy(&old_hid_in_report, new_hid_in_report, sizeof(struct hid_in_report_data_boot));

	// For Boot Protocol we can only send the first 8 bytes
	if(!hid_protocol)
		len = 8;
	else
		len = sizeof(struct hid_in_report_data_boot);

	if(usbd_ep_write_packet(dev, HID_ENDPOINT_IN_ADDR_BOOT, (void *)new_hid_in_report, len))
		hid_report_transmitting_boot = 1;
}

static void remote_wakeup_key_event_callback(
	uint8_t column,
	uint8_t row,
	uint8_t state,
	uint8_t pressed,
	uint8_t released,
	void *data
) {
	uint8_t *any_key_pressed;

	(void)column;
	(void)row;
	(void)state;
	(void)released;

	any_key_pressed = (uint8_t *)data;

	if(pressed)
		*any_key_pressed = 1;
}

void hid_poll(usbd_device *dev) {
	struct hid_in_report_data_boot new_hid_in_report;
	uint32_t now;

	if(!usbd_is_suspended())
		hid_usbd_remote_wakeup_sent = 0;

	if(usbd_state == USBD_STATE_CONFIGURED && usbd_is_suspended() && usbd_remote_wakeup_enabled && !hid_usbd_remote_wakeup_sent) {
		uint8_t any_key_pressed;

		any_key_pressed = 0;
		keys_scan(remote_wakeup_key_event_callback, &any_key_pressed);
		if(any_key_pressed){
			usdb_remote_wakeup_signal();
			hid_usbd_remote_wakeup_sent = 1;
		}
		return;
	}

	if(!(usbd_state == USBD_STATE_CONFIGURED && !usbd_is_suspended()))
		return;

	if(hid_report_transmitting_boot)
		return;

	// Only send if there are changes
	if(hid_idle_rate_ms == 0) {
		get_hid_in_report(&new_hid_in_report);
		if(memcmp(&new_hid_in_report, &old_hid_in_report, sizeof(struct hid_in_report_data_boot)))
			send_in_report(dev, &new_hid_in_report);
	// Only send if there are changes or at idle rate
	} else {
		get_hid_in_report(&new_hid_in_report);
		now = uptime_ms();
		if(now >= idle_finish_ms) {
			send_in_report(dev, &new_hid_in_report);
			idle_finish_ms = now + hid_idle_rate_ms;
		} else {
			if(memcmp(&new_hid_in_report, &old_hid_in_report, sizeof(struct hid_in_report_data_boot)))
				send_in_report(dev, &new_hid_in_report);
		}
	}
}