#include "keys.h"
#include "keys/scan.h"
#include "keys/layers.h"
#include "keys/sequence.h"
#include <descriptors.h>
#include <libopencm3/usb/hid_usage_tables.h>

static uint8_t layer_state[LAYER_COUNT];

static void load_layer_state(void) {
	for(uint8_t layer_idx=0 ; layer_idx < LAYER_COUNT ; layer_idx++) {
		switch(layers_default_state[layer_idx]) {
			case KEYS_LAYER_STATE_ENABLED:
				layer_state[layer_idx] = 1;
				break;
			case KEYS_LAYER_STATE_DISABLED:
				layer_state[layer_idx] = 0;
				break;
			case KEYS_LAYER_STATE_LOAD:
				// TODO load config
				layer_state[layer_idx] = (layer_idx == 4);
				break;
		}
	}
}

void keys_setup() {
	keys_scan_setup();
	load_layer_state();
	sequence_init();
}

void keys_reset() {
	// TODO reset keyboard state: active layers, etc
	keys_scan_reset();
	load_layer_state();
}

static void key_event_callback(uint8_t row, uint8_t column, uint8_t state, uint8_t pressed, uint8_t released, void *data) {
	struct hid_in_report_data *hid_in_report;
	uint16_t hid_usage_page, hid_usage_id;
	uint8_t layer_idx;

	hid_in_report = (struct hid_in_report_data *)data;

	layer_idx=0;
	retry:

	for(; layer_idx < LAYER_COUNT ; layer_idx++)
		if(layer_state[layer_idx])
			break;
	if(layer_idx < LAYER_COUNT) {
		hid_usage_page = layers_keymap[layer_idx][row][column].page;
		hid_usage_id = layers_keymap[layer_idx][row][column].id;
	} else {
		hid_usage_page = USB_HID_USAGE_PAGE_NONE;
		hid_usage_id = 0;
	}

	// Regular HID Usage Tables
	if(hid_usage_page < 0xFF00) {
		if(state)
			hid_in_report_add(hid_in_report, hid_usage_page, hid_usage_id);
	// Vendor Defined HID Usage Tables
	} else {
		(void)released;
		switch(hid_usage_page) {
			case USB_HID_USAGE_PAGE_NONE:
				break;
			case USB_HID_USAGE_PAGE_NEXT_LAYER:
				layer_idx++;
				goto retry;
			case USB_HID_USAGE_PAGE_FUNCTION:
				// TODO
				break;
			case USB_HID_USAGE_PAGE_SEQUENCE:
				if(pressed)
					sequence_register(sequences[hid_usage_id]);
				break;
		}
	}
}

void keys_populate_hid_in_report(struct hid_in_report_data *hid_in_report) {
	sequence_play(hid_in_report);
	keys_scan(key_event_callback, (void *)hid_in_report);
}