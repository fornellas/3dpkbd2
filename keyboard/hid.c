#include "descriptors.h"
#include "hid.h"
#include "lib/systick.h"
#include "usb.h"
#include <stdlib.h>
#include <string.h>
#include "keys.h"
#include <libopencm3/usb/hid_usage_tables.h>

//
// Variables
//

uint8_t hid_protocol = USB_HID_PROTOCOL_REPORT;
uint16_t hid_idle_rate_ms_boot = 0;
uint16_t hid_idle_rate_ms_extra = 0;
hid_out_report_data_boot_t hid_led_report;

static uint32_t idle_finish_ms_boot = 0;
static uint32_t idle_finish_ms_extra = 0;
static uint8_t hid_report_transmitting_boot = 0;
static uint8_t hid_report_transmitting_extra = 0;
static struct hid_in_report_data_boot_t old_hid_in_report_data_boot;
static struct hid_in_report_data_extra_t old_hid_in_report_data_extra;
static uint8_t hid_usbd_remote_wakeup_sent = 0;

static void send_hid_in_report_data_boot(usbd_device *dev, struct hid_in_report_data_boot_t *);
static void send_hid_in_report_data_extra(usbd_device *dev, struct hid_in_report_data_extra_t *);

//
// Standard HID Requests
//

static enum usbd_request_return_codes hid_standard_get_descriptor(
	uint8_t descriptor_type,
	uint8_t descriptor_index,
	uint8_t interface_number,
	uint8_t **buf,
	uint16_t *len
) {
	(void)descriptor_index;

	switch(interface_number) {
		case HID_INTERFACE_NUMBER_BOOT:
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
			break;
		case HID_INTERFACE_NUMBER_EXTRA:
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
			break;
	}

	return USBD_REQ_NOTSUPP;
}

static enum usbd_request_return_codes hid_standard_request(
	usbd_device *dev,
	struct usb_setup_data *req,
	uint8_t **buf,
	uint16_t *len,
	void (**complete)(usbd_device *, struct usb_setup_data *)
) {
	uint8_t descriptor_type;
	uint8_t descriptor_index;
	uint8_t interface_number;

	(void)dev;
	(void)complete;

	descriptor_type = req->wValue >> 8;
	descriptor_index = req->wValue & 0xFF;
	interface_number = req->wIndex;

	// 7.1.1 Get_Descriptor Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == USB_REQ_GET_DESCRIPTOR)
	) {
		return hid_standard_get_descriptor(
			descriptor_type,
			descriptor_index,
			interface_number,
			buf,
			len
		);
	}

	// 7.1.2 Set_Descriptor Request
	// if(
	// 	((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
	// 	&& (req->bRequest == USB_REQ_SET_DESCRIPTOR)
	// ) {
	// }

	return USBD_REQ_NEXT_CALLBACK;
}

//
// HID Class Requests
//

static void populate_hid_in_report_data_boot(
	struct hid_usage_list_t *hid_usage_list,
	struct hid_in_report_data_boot_t *new_hid_in_report_data_boot
) {
	memset(new_hid_in_report_data_boot, 0, sizeof(struct hid_in_report_data_boot_t));
	for(uint8_t hid_usage_list_idx=0 ; hid_usage_list_idx < MAX_HID_USAGE_KEYS ; hid_usage_list_idx++) {
		uint16_t hid_usage_page;
		uint16_t hid_usage_id;

		hid_usage_page = hid_usage_list->values[hid_usage_list_idx].page;
		hid_usage_id = hid_usage_list->values[hid_usage_list_idx].id;

		if(hid_usage_page != USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD)
			continue;

		if(
			hid_usage_id >= USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_LEFT_CONTROL
			&& hid_usage_id <= USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_RIGHT_GUI
		) {
			uint8_t modifier_bit;

			modifier_bit = hid_usage_id - USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_LEFT_CONTROL;
			new_hid_in_report_data_boot->keyboard_keypad_modifiers |= (1 << modifier_bit);
		} else {
			uint8_t error_roll_over;

			error_roll_over = 1;
			for(uint8_t i=0 ; i < KEYBOARD_PAGE_MAX ; i++) {
				if(new_hid_in_report_data_boot->keyboard_keypad[i] != USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_RESERVED_NO_EVENT_INDICATED){
					continue;
				} else {
					new_hid_in_report_data_boot->keyboard_keypad[i] = hid_usage_id;
					error_roll_over = 0;
					break;
				}
			}
			if(error_roll_over)
				for(uint8_t i=0 ; i < KEYBOARD_PAGE_MAX ; i++)
					new_hid_in_report_data_boot->keyboard_keypad[i] = USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_ERROR_ROLLOVER;
		}
	}
}

static void populate_hid_in_report_data_extra(
	struct hid_usage_list_t *hid_usage_list,
	struct hid_in_report_data_extra_t *new_hid_in_report_data_extra
) {
	memset(new_hid_in_report_data_extra, 0, sizeof(struct hid_in_report_data_extra_t));
	for(uint8_t hid_usage_list_idx=0 ; hid_usage_list_idx < MAX_HID_USAGE_KEYS ; hid_usage_list_idx++) {
		uint16_t hid_usage_page;
		uint16_t hid_usage_id;

		hid_usage_page = hid_usage_list->values[hid_usage_list_idx].page;
		hid_usage_id = hid_usage_list->values[hid_usage_list_idx].id;

		switch(hid_usage_page) {
			case USB_HID_USAGE_PAGE_GENERIC_DESKTOP:
				for(uint8_t i=0 ; i < 6 ; i++) {
					if(!new_hid_in_report_data_extra->generic_desktop[i]) {
						new_hid_in_report_data_extra->generic_desktop[i] = hid_usage_id;
						break;
					}
				}
				break;
			case USB_HID_USAGE_PAGE_CONSUMER:
				for(uint8_t i=0 ; i < 6 ; i++) {
					if(!new_hid_in_report_data_extra->consumer_devices[i]) {
						new_hid_in_report_data_extra->consumer_devices[i] = hid_usage_id;
						break;
					}
				}
				break;
		}
	}

}

static enum usbd_request_return_codes hid_class_get_report(
	uint8_t report_type,
	uint8_t report_id,
	uint8_t interface_number,
	uint8_t report_length,
	uint8_t **buf,
	uint16_t *len
) {
	struct hid_usage_list_t hid_usage_list;
	static struct hid_in_report_data_boot_t new_hid_in_report_data_boot;
	static struct hid_in_report_data_extra_t new_hid_in_report_data_extra;

	(void)report_id;
	(void)report_length;

	switch(report_type) {
		case USB_HID_REPORT_TYPE_INPUT:
			keys_populate_hid_usage_list(&hid_usage_list);
			switch(interface_number) {
				case HID_INTERFACE_NUMBER_BOOT:
					populate_hid_in_report_data_boot(&hid_usage_list, &new_hid_in_report_data_boot);
					memcpy(*buf, &new_hid_in_report_data_boot, sizeof(struct hid_in_report_data_boot_t));;
					*len = sizeof(struct hid_in_report_data_boot_t);
					return USBD_REQ_HANDLED;
				case HID_INTERFACE_NUMBER_EXTRA:
					populate_hid_in_report_data_extra(&hid_usage_list, &new_hid_in_report_data_extra);
					memcpy(*buf, &new_hid_in_report_data_extra, sizeof(struct hid_in_report_data_extra_t));;
					*len = sizeof(struct hid_in_report_data_extra_t);
					return USBD_REQ_HANDLED;
			}
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

	switch(report_type) {
		// case USB_HID_REPORT_TYPE_INPUT:
		// 	break;
		case USB_HID_REPORT_TYPE_OUTPUT:
			if (interface_number == HID_INTERFACE_NUMBER_BOOT) {
				if (report_length != sizeof(hid_led_report))
					return USBD_REQ_NOTSUPP;
				// FIXME check len
				hid_led_report = *buf[0];
				return USBD_REQ_HANDLED;
			}
			break;
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
	switch(interface_number) {
		case HID_INTERFACE_NUMBER_BOOT:
			if(report_id == 0) {
				*buf[0] = hid_idle_rate_ms_boot / 4;
				*len = sizeof(uint8_t);
				return USBD_REQ_HANDLED;
			}
			break;
		case HID_INTERFACE_NUMBER_EXTRA:
			if(report_id == 0) {
				*buf[0] = hid_idle_rate_ms_extra / 4;
				*len = sizeof(uint8_t);
				return USBD_REQ_HANDLED;
			}
			break;
	}

	return USBD_REQ_NOTSUPP;
}

static enum usbd_request_return_codes hid_class_set_idle(
	uint16_t duration_ms,
	uint8_t report_id,
	uint8_t interface_number
) {
	switch(interface_number) {
		case HID_INTERFACE_NUMBER_BOOT:
			if(report_id == 0) {
				hid_idle_rate_ms_boot = duration_ms;
				idle_finish_ms_boot = uptime_ms() + hid_idle_rate_ms_boot;
				return USBD_REQ_HANDLED;
			}
			break;
		case HID_INTERFACE_NUMBER_EXTRA:
			if(report_id == 0) {
				hid_idle_rate_ms_extra = duration_ms;
				idle_finish_ms_extra = uptime_ms() + hid_idle_rate_ms_extra;
				return USBD_REQ_HANDLED;
			}
			break;
	}

	return USBD_REQ_NOTSUPP;
}

static enum usbd_request_return_codes hid_class_get_protocol(
	uint8_t interface_number,
	uint8_t **buf,
	uint16_t *len
) {
	if (interface_number != HID_ENDPOINT_NUMBER_BOOT)
		return USBD_REQ_NOTSUPP;

	*buf = (uint8_t *)&hid_protocol;
	*len = sizeof(hid_protocol);
	return USBD_REQ_HANDLED;
}

static enum usbd_request_return_codes hid_class_set_protocol(
	uint8_t protocol,
	uint8_t interface_number
) {
	if (interface_number != HID_ENDPOINT_NUMBER_BOOT)
		return USBD_REQ_NOTSUPP;

	switch(hid_protocol) {
		case 0:  // Boot Protocol
		case 1:  // Report Protocol
			hid_protocol = protocol;
			return USBD_REQ_HANDLED;
	}
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
		return hid_class_set_idle(
			(uint16_t)(req->wValue >> 8) * 4,
			req->wValue & 0xFF,
			interface_number
		);
	}

	// 7.2.5 Get_Protocol Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == USB_HID_REQ_TYPE_GET_PROTOCOL)
	) {
		return hid_class_get_protocol(
			interface_number,
			buf,
			len
		);
	}

	// 7.2.6 Set_Protocol Request
	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
		&& (req->bRequest == USB_HID_REQ_TYPE_SET_PROTOCOL)
	) {
		return hid_class_set_protocol(
			req->wValue,
			interface_number
		);
	}

	return USBD_REQ_NEXT_CALLBACK;
}

//
//
//

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
	struct hid_usage_list_t hid_usage_list;

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
	hid_idle_rate_ms_boot = 0;
	hid_idle_rate_ms_extra = 0;
	idle_finish_ms_boot = 0;
	idle_finish_ms_extra = 0;
	hid_report_transmitting_boot = 0;
	hid_report_transmitting_extra = 0;
	keys_reset();
	keys_populate_hid_usage_list(&hid_usage_list);
	populate_hid_in_report_data_boot(&hid_usage_list, &old_hid_in_report_data_boot);
	populate_hid_in_report_data_extra(&hid_usage_list, &old_hid_in_report_data_extra);
	hid_usbd_remote_wakeup_sent = 0;
}

static void send_hid_in_report_data_boot(
	usbd_device *dev,
	struct hid_in_report_data_boot_t *hid_in_report_data_boot
) {
	memcpy(
		&old_hid_in_report_data_boot,
		hid_in_report_data_boot,
		sizeof(struct hid_in_report_data_boot_t)
	);
	if(
		usbd_ep_write_packet(
			dev, HID_ENDPOINT_IN_ADDR_BOOT,
			(void *)&old_hid_in_report_data_boot,
			sizeof(struct hid_in_report_data_boot_t)
		)
	)
		hid_report_transmitting_boot = 1;
}

static void send_hid_in_report_data_extra(
	usbd_device *dev,
	struct hid_in_report_data_extra_t *hid_in_report_data_extra
) {
	memcpy(
		&old_hid_in_report_data_extra,
		hid_in_report_data_extra,
		sizeof(struct hid_in_report_data_extra_t)
	);
	if(
		usbd_ep_write_packet(
			dev, HID_ENDPOINT_IN_ADDR_BOOT,
			(void *)&old_hid_in_report_data_extra,
			sizeof(struct hid_in_report_data_extra_t)
		)
	)
		hid_report_transmitting_extra = 1;
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
	struct hid_usage_list_t hid_usage_list;
	struct hid_in_report_data_boot_t hid_in_report_data_boot;
	struct hid_in_report_data_extra_t hid_in_report_data_extra;
	uint32_t now;

	//
	// Suspended
	//

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

	//
	// Reports
	//

	if(hid_report_transmitting_boot || hid_report_transmitting_extra)
		return;

	keys_populate_hid_usage_list(&hid_usage_list);
	populate_hid_in_report_data_boot(&hid_usage_list, &hid_in_report_data_boot);
	populate_hid_in_report_data_extra(&hid_usage_list, &hid_in_report_data_extra);

	// Only send if there are changes
	if(hid_idle_rate_ms_boot == 0) {
		if(
			memcmp(
				&hid_in_report_data_boot, &old_hid_in_report_data_boot,
				sizeof(struct hid_in_report_data_boot_t)
			)
		)
			send_hid_in_report_data_boot(dev, &hid_in_report_data_boot);
	// Only send if there are changes or at idle rate
	} else {
		now = uptime_ms();
		if(now >= idle_finish_ms_boot) {
			send_hid_in_report_data_boot(dev, &hid_in_report_data_boot);
			idle_finish_ms_boot = now + hid_idle_rate_ms_boot;
		} else {
			if(
				memcmp(
					&hid_in_report_data_boot, &old_hid_in_report_data_boot,
					sizeof(struct hid_in_report_data_boot_t)
				)
			)
				send_hid_in_report_data_boot(dev, &hid_in_report_data_boot);
		}
	}
}

uint8_t hid_usage_list_add(
	struct hid_usage_list_t *hid_usage_list,
	uint16_t page, uint16_t id
) {
	for(uint8_t i=0 ; i < MAX_HID_USAGE_KEYS ; i++) {
		if(
			!(hid_usage_list->values[i].page)
			&& !(hid_usage_list->values[i].id)
		) {
			hid_usage_list->values[i].page = page;
			hid_usage_list->values[i].id = id;
			return 0;
		}
	}
	return 1;
}