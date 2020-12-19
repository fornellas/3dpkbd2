#include "keys.h"
#include "keys/scan.h"
#include "keys/layers.h"
#include "keys/sequence.h"
#include <descriptors.h>
#include <libopencm3/usb/hid_usage_tables.h>
#include <string.h>
#include <stdbool.h>

bool do_shifted_keyboard_keypad_update;

static void load_layer_state(void) {
	for(uint8_t layer_idx=0 ; layer_idx < LAYER_COUNT ; layer_idx++) {
		switch(layers_default_state[layer_idx]) {
			case KEYS_LAYER_STATE_ENABLED:
				layers_state[layer_idx] = 1;
				break;
			case KEYS_LAYER_STATE_DISABLED:
				layers_state[layer_idx] = 0;
				break;
			case KEYS_LAYER_STATE_LOAD:
				// TODO load config
				layers_state[layer_idx] = (layer_idx == LAYER_DVORAK_DVORAK); // FIXME
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
	keys_scan_state_reset();
	load_layer_state();
	sequence_reset();
}

static void key_event_callback(
	uint8_t row,
	uint8_t column,
	uint8_t state,
	uint8_t pressed,
	uint8_t released,
	void *data
) {
	struct hid_usage_list_t *hid_usage_list;
	uint16_t hid_usage_page, hid_usage_id;
	uint8_t layer_idx;

	hid_usage_list = (struct hid_usage_list_t *)data;

	layer_idx=0;
	retry:

	for(; layer_idx < LAYER_COUNT ; layer_idx++)
		if(layers_state[layer_idx])
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
			hid_usage_list_add(hid_usage_list, hid_usage_page, hid_usage_id);
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
				functions[hid_usage_id](row, column, state, pressed, released, hid_usage_list);
				break;
			case USB_HID_USAGE_PAGE_SEQUENCE:
				if(pressed)
					sequence_register(sequences[hid_usage_id]);
				break;
			case USB_HID_USAGE_PAGE_LAYOUT:
				if(pressed)
					layout_set(hid_usage_id);
				break;
			case USB_HID_USAGE_PAGE_TOGGLE_LAYER:
				if(pressed)
					toggle_layer(hid_usage_id);
				break;
			case USB_HID_USAGE_PAGE_SHIFTED_KEYBOARD_KEYPAD:
				if(state) {
					do_shifted_keyboard_keypad_update = true;
					hid_usage_list_add(
						hid_usage_list,
						USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD,
						hid_usage_id
					);
				}
				break;
		}
	}
}

void shifted_keyboard_keypad_update(struct hid_usage_list_t *hid_usage_list);

void shifted_keyboard_keypad_update(struct hid_usage_list_t *hid_usage_list) {
	if(
		hid_usage_list_has(
			hid_usage_list,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_LEFT_CONTROL
		) ||
		hid_usage_list_has(
			hid_usage_list,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_LEFT_ALT
		) ||
		hid_usage_list_has(
			hid_usage_list,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_LEFT_GUI
		) ||
		hid_usage_list_has(
			hid_usage_list,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_RIGHT_CONTROL
		) ||
		hid_usage_list_has(
			hid_usage_list,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_RIGHT_ALT
		) ||
		hid_usage_list_has(
			hid_usage_list,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_RIGHT_GUI
		)
	) {
		return;
	}
	if(
		hid_usage_list_has(
			hid_usage_list,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_LEFT_SHIFT
		) ||
		hid_usage_list_has(
			hid_usage_list,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_RIGHT_SHIFT
		)
	) {
		hid_usage_list_remove(
			hid_usage_list,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_LEFT_SHIFT
		);
		hid_usage_list_remove(
			hid_usage_list,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_RIGHT_SHIFT
		);
	} else {
		hid_usage_list_add(
			hid_usage_list,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD,
			USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_LEFT_SHIFT
		);
	}
}

void keys_populate_hid_usage_list(struct hid_usage_list_t *hid_usage_list) {
	memset(hid_usage_list, 0, sizeof(struct hid_usage_list_t));
	sequence_play(hid_usage_list);
	do_shifted_keyboard_keypad_update = false;
	keys_scan(key_event_callback, (void *)hid_usage_list);
	if(do_shifted_keyboard_keypad_update)
		shifted_keyboard_keypad_update(hid_usage_list);
}