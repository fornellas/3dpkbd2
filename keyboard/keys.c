#include "keys.h"
#include "keys_scan.h"
#include <descriptors.h>
#include <libopencm3/usb/hid_usage_tables.h>

void keys_setup() {
	keys_scan_setup();
}

void keys_reset() {

}

static void get_hid_usage(uint8_t column, uint8_t row, uint16_t *hid_usage_page, uint16_t *hid_usage_id) {
	(void)column;
	(void)row;
	// TODO process active layers
	*hid_usage_page = USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD;
	*hid_usage_id = USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_A;
}

static void key_event_callback(uint8_t column, uint8_t row, uint8_t state, uint8_t pressed, uint8_t released, void *data) {
	struct hid_in_report_data *hid_in_report;
	uint16_t hid_usage_page, hid_usage_id;

	hid_in_report = (struct hid_in_report_data *)data;

	get_hid_usage(column, row, &hid_usage_page, &hid_usage_id);

	// Regular HID Usage Tables
	if(hid_usage_page < 0xFF00) {
		if(state)
			hid_in_report_add(hid_in_report, hid_usage_page, hid_usage_id);
	// Reserved HID Usage Tables
	} else {
		(void)pressed;
		(void)released;
		// TODO layout
		// TODO macro
		// TODO sequence
	}
}

void keys_populate_hid_in_report(struct hid_in_report_data *hid_in_report) {
	// TODO play sequences
	keys_scan(key_event_callback, (void *)hid_in_report);
}