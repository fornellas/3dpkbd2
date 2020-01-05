#include "keys.h"
#include "keys/scan.h"
#include "keys/layers.h"
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
			case KEYS_LAYER_STATE_CONFIG:
				// TODO load config
				layer_state[layer_idx] = (layer_idx == 4);
				break;
		}
	}
}

void keys_setup() {
	keys_scan_setup();
	load_layer_state();
}

void keys_reset() {
	// TODO reset keyboard state: active layers, etc
	keys_scan_reset();
	load_layer_state();
}

static void get_hid_usage(uint8_t row, uint8_t column, uint16_t *hid_usage_page, uint16_t *hid_usage_id) {
	uint8_t layer_idx;
	const struct keys_hid_usage_data *hid_usage;

	for(layer_idx=0 ; layer_idx < LAYER_COUNT ; layer_idx++)
		if(layer_state[layer_idx])
			break;
	// FIXME map no active layer to no action
	#pragma GCC diagnostic ignored "-Warray-bounds"
	hid_usage = &(layers_keymap[layer_idx][row][column]);
	#pragma GCC diagnostic pop
	*hid_usage_page = hid_usage->page;
	*hid_usage_id = hid_usage->id;
}

static void key_event_callback(uint8_t row, uint8_t column, uint8_t state, uint8_t pressed, uint8_t released, void *data) {
	struct hid_in_report_data *hid_in_report;
	uint16_t hid_usage_page, hid_usage_id;

	hid_in_report = (struct hid_in_report_data *)data;

	get_hid_usage(row, column, &hid_usage_page, &hid_usage_id);

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