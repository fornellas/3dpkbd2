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

static uint8_t hid_usbd_remote_wakeup_sent = 0;

uint8_t hid_protocol = USB_HID_PROTOCOL_REPORT;

uint16_t hid_idle_rate_ms_boot = 0;
static uint32_t idle_finish_ms_boot = 0;
static uint8_t hid_report_transmitting_boot = 0;
static struct hid_in_report_boot_t *next_hid_in_report_boot=NULL;
static struct hid_in_report_boot_t old_hid_in_report_boot;
hid_out_report_boot_t hid_led_report;

uint16_t hid_idle_rate_ms_extra = 0;
static uint32_t idle_finish_ms_extra = 0;
static uint8_t hid_report_transmitting_extra = 0;
static struct hid_in_report_extra_t *next_hid_in_report_extra=NULL;
static struct hid_in_report_extra_t old_hid_in_report_extra;


static bool should_send_hid_in_report_boot(struct hid_in_report_boot_t *hid_in_report_boot, bool *discard);
static bool should_send_hid_in_report_extra(struct hid_in_report_extra_t *hid_in_report_extra, bool *discard);

static uint8_t send_hid_in_report_boot(usbd_device *dev, struct hid_in_report_boot_t *hid_in_report_boot);
static uint8_t send_hid_in_report_extra(usbd_device *dev, struct hid_in_report_extra_t *hid_in_report_extra);

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

static void populate_hid_in_report_boot(
	struct hid_usage_list_t *hid_usage_list,
	struct hid_in_report_boot_t *new_hid_in_report_boot
) {
	memset(new_hid_in_report_boot, 0, sizeof(struct hid_in_report_boot_t));
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
			new_hid_in_report_boot->keyboard_keypad_modifiers |= (1 << modifier_bit);
		} else {
			uint8_t error_roll_over;

			error_roll_over = 1;
			for(uint8_t i=0 ; i < KEYBOARD_PAGE_MAX ; i++) {
				if(new_hid_in_report_boot->keyboard_keypad[i] != USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_RESERVED_NO_EVENT_INDICATED){
					continue;
				} else {
					new_hid_in_report_boot->keyboard_keypad[i] = hid_usage_id;
					error_roll_over = 0;
					break;
				}
			}
			if(error_roll_over)
				for(uint8_t i=0 ; i < KEYBOARD_PAGE_MAX ; i++)
					new_hid_in_report_boot->keyboard_keypad[i] = USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_ERROR_ROLLOVER;
		}
	}
}

static void populate_hid_in_report_extra(
	struct hid_usage_list_t *hid_usage_list,
	struct hid_in_report_extra_t *new_hid_in_report_extra
) {
	memset(new_hid_in_report_extra, 0, sizeof(struct hid_in_report_extra_t));
	for(uint8_t hid_usage_list_idx=0 ; hid_usage_list_idx < MAX_HID_USAGE_KEYS ; hid_usage_list_idx++) {
		uint16_t hid_usage_page;
		uint16_t hid_usage_id;

		hid_usage_page = hid_usage_list->values[hid_usage_list_idx].page;
		hid_usage_id = hid_usage_list->values[hid_usage_list_idx].id;

		switch(hid_usage_page) {
			case USB_HID_USAGE_PAGE_CONSUMER:
				for(uint8_t i=0 ; i < CONSUMER_DEVICES_PAGE_MAX ; i++) {
					if(!new_hid_in_report_extra->consumer_devices[i]) {
						new_hid_in_report_extra->consumer_devices[i] = hid_usage_id;
						break;
					}
				}
				break;
		}
	}
}

static void update_hid_in_reports(bool force) {
	static struct hid_usage_list_t hid_usage_list;
	static struct hid_in_report_boot_t hid_in_report_boot;
	static struct hid_in_report_extra_t hid_in_report_extra;

	if(force || (next_hid_in_report_boot == NULL && next_hid_in_report_extra == NULL)) {
		keys_populate_hid_usage_list(&hid_usage_list);

		populate_hid_in_report_boot(&hid_usage_list, &hid_in_report_boot);
		next_hid_in_report_boot = &hid_in_report_boot;

		populate_hid_in_report_extra(&hid_usage_list, &hid_in_report_extra);
		next_hid_in_report_extra = &hid_in_report_extra;
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
	static struct hid_in_report_boot_t new_hid_in_report_boot;
	static struct hid_in_report_extra_t new_hid_in_report_extra;

	(void)report_id;
	(void)report_length;

	switch(report_type) {
		case USB_HID_REPORT_TYPE_INPUT:
			update_hid_in_reports(true);
			switch(interface_number) {
				case HID_INTERFACE_NUMBER_BOOT:
					memcpy(*buf, next_hid_in_report_boot, sizeof(struct hid_in_report_boot_t));
					*len = sizeof(struct hid_in_report_boot_t);
					return USBD_REQ_HANDLED;
				case HID_INTERFACE_NUMBER_EXTRA:
					memcpy(*buf, next_hid_in_report_extra, sizeof(struct hid_in_report_extra_t));
					*len = sizeof(struct hid_in_report_extra_t);
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

	hid_usbd_remote_wakeup_sent = 0;

	hid_protocol = USB_HID_PROTOCOL_REPORT;

	hid_idle_rate_ms_boot = 0;
	idle_finish_ms_boot = 0;
	hid_report_transmitting_boot = 0;
	memset(&old_hid_in_report_boot, 0, sizeof(struct hid_in_report_boot_t));

	hid_idle_rate_ms_extra = 0;
	idle_finish_ms_extra = 0;
	hid_report_transmitting_extra = 0;
	memset(&old_hid_in_report_extra, 0, sizeof(struct hid_in_report_extra_t));

	keys_reset();
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

static bool should_send_hid_in_report_boot(struct hid_in_report_boot_t *hid_in_report_boot, bool *discard) {
	uint32_t now;

	if(hid_in_report_boot == NULL){
		*discard = true;
		return false;
	}

	if(hid_idle_rate_ms_boot == 0) {
		// Send if there are changes
		if(memcmp(hid_in_report_boot, &old_hid_in_report_boot, sizeof(struct hid_in_report_boot_t))){
			*discard = false;
			return true;
		} else {
			*discard = true;
			return false;
		}
	} else {
		now = uptime_ms();
		// Send at idle rate
		if(now >= idle_finish_ms_boot) {
			idle_finish_ms_boot = now + hid_idle_rate_ms_boot;
			*discard = false;
			return true;
		} else {
			// Send if there are changes
			if(memcmp(hid_in_report_boot, &old_hid_in_report_boot, sizeof(struct hid_in_report_boot_t))){
				*discard = false;
				return true;
			} else {
				*discard = false;
				return false;
			}
		}
	}
}

static bool should_send_hid_in_report_extra(struct hid_in_report_extra_t *hid_in_report_extra, bool *discard) {
	uint32_t now;

	if(hid_in_report_extra == NULL){
		*discard = true;
		return false;
	}

	if(hid_idle_rate_ms_extra == 0) {
		// Send if there are changes
		if(memcmp(hid_in_report_extra, &old_hid_in_report_extra, sizeof(struct hid_in_report_extra_t))){
			*discard = false;
			return true;
		} else {
			*discard = true;
			return false;
		}
	} else {
		now = uptime_ms();
		// Send at idle rate
		if(now >= idle_finish_ms_extra) {
			idle_finish_ms_extra = now + hid_idle_rate_ms_extra;
			*discard = false;
			return true;
		} else {
			// Send if there are changes
			if(memcmp(hid_in_report_extra, &old_hid_in_report_extra, sizeof(struct hid_in_report_extra_t))){
				*discard = false;
				return true;
			} else {
				*discard = false;
				return false;
			}
		}
	}
}

static uint8_t send_hid_in_report_boot(
	usbd_device *dev,
	struct hid_in_report_boot_t *hid_in_report_boot
) {
	memcpy(
		&old_hid_in_report_boot,
		hid_in_report_boot,
		sizeof(struct hid_in_report_boot_t)
	);
	if(
		usbd_ep_write_packet(
			dev, HID_ENDPOINT_IN_ADDR_BOOT,
			(void *)&old_hid_in_report_boot,
			sizeof(struct hid_in_report_boot_t)
		)
	) {
		return 1;
	}
	return 0;
}

static uint8_t send_hid_in_report_extra(
	usbd_device *dev,
	struct hid_in_report_extra_t *hid_in_report_extra
) {
	memcpy(
		&old_hid_in_report_extra,
		hid_in_report_extra,
		sizeof(struct hid_in_report_extra_t)
	);
	if(
		usbd_ep_write_packet(
			dev, HID_ENDPOINT_IN_ADDR_EXTRA,
			(void *)&old_hid_in_report_extra,
			sizeof(struct hid_in_report_extra_t)
		)
	) {
		return 1;
	}
	return 0;
}

void hid_poll(usbd_device *dev) {
	bool discard;

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

	update_hid_in_reports(false);

	if(should_send_hid_in_report_boot(next_hid_in_report_boot, &discard)) {
		if(!hid_report_transmitting_boot){
			if(send_hid_in_report_boot(dev, next_hid_in_report_boot)){
				hid_report_transmitting_boot = 1;
				next_hid_in_report_boot = NULL;
			}
		}
	} else if(discard) {
		next_hid_in_report_boot = NULL;
	}

	if(should_send_hid_in_report_extra(next_hid_in_report_extra, &discard)) {
		if(!hid_report_transmitting_extra){
			if(send_hid_in_report_extra(dev, next_hid_in_report_extra)){
				hid_report_transmitting_extra = 1;
				next_hid_in_report_extra = NULL;
			}
		}
	} else if(discard) {
		next_hid_in_report_extra = NULL;
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

bool hid_usage_list_has(
	struct hid_usage_list_t *hid_usage_list,
	uint16_t page, uint16_t id
) {
	for(uint8_t i=0 ; i < MAX_HID_USAGE_KEYS ; i++) {
		if(
			hid_usage_list->values[i].page == page &&
			hid_usage_list->values[i].id == id
		) {
			return true;
		}
	}
	return false;
}

void hid_usage_list_remove(
	struct hid_usage_list_t *hid_usage_list,
	uint16_t page, uint16_t id
) {
	for(uint8_t i=0 ; i < MAX_HID_USAGE_KEYS ; i++) {
		if(
			hid_usage_list->values[i].page == page &&
			hid_usage_list->values[i].id == id
		) {
			hid_usage_list->values[i].page = 0;
			hid_usage_list->values[i].id = 0;
			return;
		}
	}
}