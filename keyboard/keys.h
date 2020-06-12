#ifndef KEYS_H
#define KEYS_H

#include "descriptors.h"
#include "hid.h"
#include <stdint.h>

#define USB_HID_USAGE_PAGE_NONE 0xFF00
#define USB_HID_USAGE_PAGE_NEXT_LAYER 0xFF01
#define USB_HID_USAGE_PAGE_FUNCTION 0xFF02
#define USB_HID_USAGE_PAGE_SEQUENCE 0xFF03
#define USB_HID_USAGE_PAGE_LAYOUT 0xFF04

enum keys_layer_states {
  KEYS_LAYER_STATE_ENABLED,
  KEYS_LAYER_STATE_DISABLED,
  KEYS_LAYER_STATE_LOAD,
};

void keys_setup(void);

void keys_reset(void);

void keys_populate_hid_usage_list(struct hid_usage_list_t *);

void keys_scan(void (*)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, void *), void *);

extern uint32_t layout_changes_counter;

#endif