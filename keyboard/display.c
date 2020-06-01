#include "display.h"
#include "hid.h"
#include "usb.h"
#include "keys/scan.h"
#include "lib/systick.h"
#include "lib/images/boot.h"
#include "lib/images/print_head.h"
#include "lib/images/report.h"
#include "lib/images/right_side_disconnected.h"
#include <stdio.h>
#include <string.h>

#define TOGGLE_WIDTH 24
#define TOGGLE_HEIGHT 20

static ucg_t *ucg;

void display_draw_toggle(ucg_int_t, ucg_int_t, ucg_int_t, ucg_int_t, const char *, uint8_t);

struct display_state {
  // USB
  uint8_t usbd_state;
  uint8_t usbd_suspended;
  uint8_t usbd_remote_wakeup_enabled;

  // USB HID
  uint8_t hid_protocol;
  uint16_t hid_idle_rate_ms;
  uint8_t hid_led_num_lock;
  uint8_t hid_led_caps_lock;
  uint8_t hid_led_scroll_lock;
  uint8_t hid_led_compose;
  uint8_t hid_led_kana;

  // Layers
  // unsigned int layer_keypad;
  // unsigned int layer_shift_lock;
  // unsigned int layer_fn;
  // unsigned int layer_keyboard;
  // unsigned int layer_computer;

  // Counter
  // uint32_t counter_keys;

  // Right Side
  uint8_t keys_scan_right_side_disconnected;
} __attribute__((packed));

static struct display_state current_state;
static struct display_state last_state;

void display_draw_toggle(
    ucg_int_t x,
    ucg_int_t y,
    ucg_int_t width,
    ucg_int_t height,
    const char *str,
    uint8_t state
) {
  ucg_int_t str_x, str_y;

  ucg_DrawFrame(ucg, x, y, width, height);

  if(state) {
    ucg_DrawBox(ucg, x + 2, y + 2, width - 4, height - 4);
    ucg_SetColor(ucg, 0, 255, 255, 255);
  }

  str_x = x + (width / 2) - (ucg_GetStrWidth(ucg, str) / 2);
  str_y = y + (height / 2) + (ucg_GetFontAscent(ucg) / 2);

  ucg_DrawString(ucg, str_x, str_y, 0, str);

}

static void display_draw(void) {
  char buff[30];

  // USB
  if(!(current_state.usbd_state == USBD_STATE_CONFIGURED && !current_state.usbd_suspended)) {
    display_draw_usbd_status(
      current_state.usbd_state,
      current_state.usbd_suspended,
      current_state.usbd_remote_wakeup_enabled
    );
    ucg_SendBuffer(ucg);
    return;
  }

  // Background
  ucg_SetColor(ucg, 0, 255, 255, 255);
  ucg_DrawBox(ucg, 0, 0, ucg_GetWidth(ucg), ucg_GetHeight(ucg));

  // Logo
  ucg_DrawPixmap(ucg, 2, 2, print_head_width, print_head_height, print_head_data);

  // USB HID
  ucg_SetFont(ucg, ucg_font_helvB12_hf);
  ucg_SetColor(ucg, 0, 0, 0, 0);
  display_draw_toggle(50, 2, TOGGLE_WIDTH, TOGGLE_HEIGHT, "S", current_state.hid_led_scroll_lock);
  ucg_SetColor(ucg, 0, 0, 0, 0);
  display_draw_toggle(76, 2, TOGGLE_WIDTH, TOGGLE_HEIGHT, "1", current_state.hid_led_num_lock);
  ucg_SetColor(ucg, 0, 255, 0, 0);
  display_draw_toggle(102, 43, TOGGLE_WIDTH, TOGGLE_HEIGHT, "A", current_state.hid_led_caps_lock);
  ucg_SetFont(ucg, ucg_font_helvB08_hf);
  ucg_SetColor(ucg, 0, 0, 0, 0);
  if(current_state.hid_idle_rate_ms) {
    sprintf(buff, "Idle rate: %dms", current_state.hid_idle_rate_ms);
    ucg_DrawStringCentered(ucg, buff, 126);
  } else
    ucg_DrawStringCentered(ucg, "Idle rate: Inf", 126);
  switch(current_state.hid_protocol) {
    case USB_HID_PROTOCOL_BOOT:
      ucg_DrawPixmap(ucg, 102, 106, boot_width, boot_height, boot_data);
      break;
    case USB_HID_PROTOCOL_REPORT:
      ucg_DrawPixmap(ucg, 102, 106, report_width, report_height, report_data);
      break;
  }

  // Layers
  // ucg_SetFont(ucg, ucg_font_helvB12_hf);
  // ucg_SetColor(ucg, 0, 0, 0, 0);
  // display_draw_toggle(102, 2, TOGGLE_WIDTH, TOGGLE_HEIGHT, "K", current_state.layer_keypad);
  // ucg_SetColor(ucg, 0, 0, 0, 0);
  // display_draw_toggle(102, 65, TOGGLE_WIDTH, TOGGLE_HEIGHT, "!", current_state.layer_shift_lock);
  // ucg_SetColor(ucg, 0, 0, 0, 0);
  // display_draw_toggle(2, 106, TOGGLE_WIDTH, TOGGLE_HEIGHT, "Fn", current_state.layer_fn);

  // Counter
  // TODO

  // Right Side
  if(current_state.keys_scan_right_side_disconnected)
    ucg_DrawPixmap(ucg, 102, 87, right_side_disconnected_width, right_side_disconnected_height, right_side_disconnected_data);

  ucg_SendBuffer(ucg);
}

static void display_get_current_state(struct display_state *state) {
  // USB
  state->usbd_state = usbd_state;
  state->usbd_suspended = usbd_is_suspended();
  state->usbd_remote_wakeup_enabled = usbd_remote_wakeup_enabled;

  // USB HID
  state->hid_protocol = hid_protocol;
  state->hid_idle_rate_ms = hid_idle_rate_ms;
  // TODO move bit logic to hid.c
  state->hid_led_num_lock = hid_led_report & (1<<0);
  state->hid_led_caps_lock = hid_led_report & (1<<1);
  state->hid_led_scroll_lock = hid_led_report & (1<<2);
  state->hid_led_compose = hid_led_report & (1<<3);
  state->hid_led_kana = hid_led_report & (1<<4);

  // Layers
  // state->layer_keypad = layer_keypad;
  // state->layer_shift_lock = layer_shift_lock;
  // state->layer_fn = layer_fn;
  // state->layer_keyboard = layer_keyboard;
  // state->layer_computer = layer_computer;

  // Counter
  // state->counter_keys = counter_keys;

  // Right Side
  // state->keys_scan_right_side_disconnected = keys_scan_right_side_disconnected;
}

void display_setup(void) {
  struct display_state new_state;

  ucg = display_setup_base();

  display_get_current_state(&new_state);

  memcpy(&last_state, &new_state, sizeof(struct display_state));
  memcpy(&current_state, &new_state, sizeof(struct display_state));

  display_draw();

  ucg_SetFontMode(ucg, UCG_FONT_MODE_TRANSPARENT);
}

void display_update(void) {
  struct display_state new_state;

  display_get_current_state(&new_state);

  if(memcmp(&current_state, &new_state, sizeof(struct display_state))) {
    memcpy(&last_state, &current_state, sizeof(struct display_state));
    memcpy(&current_state, &new_state, sizeof(struct display_state));
    display_draw();
  }
}