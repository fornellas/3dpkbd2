#include "display.h"
#include "hid.h"
#include "usb.h"
#include "keys/scan.h"
#include "keys/layers.h"
#include "lib/systick.h"
#include "lib/images/boot.h"
#include "lib/images/print_head.h"
#include "lib/images/report.h"
#include "lib/images/right_side_disconnected.h"
#include <stdio.h>
#include <string.h>

#define TOGGLE_WIDTH 24
#define TOGGLE_HEIGHT 20
#define SCREENSAVER_SECS 60

static ucg_t *ucg;

void display_draw_toggle(ucg_int_t, ucg_int_t, ucg_int_t, ucg_int_t, uint8_t, uint8_t, uint8_t, const char *, uint8_t);

struct display_state {
  // USB
  uint8_t usbd_state;
  uint8_t usbd_suspended;
  uint8_t usbd_remote_wakeup_enabled;

  // USB HID
  uint8_t hid_protocol;
  uint16_t hid_idle_rate_ms_boot;
  // uint16_t hid_idle_rate_ms_extra;
  uint8_t hid_led_num_lock;
  uint8_t hid_led_caps_lock;
  uint8_t hid_led_scroll_lock;
  uint8_t hid_led_compose;
  uint8_t hid_led_kana;

  // Layer toggles
  uint8_t layer_keypad;
  uint8_t layer_shifted_number;
  uint8_t layer_fn;

  // Layout layers
  char * layer_keyboard;
  char * layer_computer;

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
    uint8_t r,
    uint8_t g,
    uint8_t b,
    const char *str,
    uint8_t state
) {
  ucg_int_t str_x, str_y;

  ucg_SetColor(ucg, 0, 224, 224, 224);
  ucg_DrawBox(ucg, x, y, width, height);
  ucg_SetColor(ucg, 0, r, g, b);
  ucg_DrawFrame(ucg, x, y, width, height);

  if(state) {
    ucg_DrawBox(ucg, x + 2, y + 2, width - 4, height - 4);
    ucg_SetColor(ucg, 0, 255, 255, 255);
  }

  str_x = x + (width / 2) - (ucg_GetStrWidth(ucg, str) / 2);
  str_y = y + (height / 2) + (ucg_GetFontAscent(ucg) / 2);

  ucg_DrawString(ucg, str_x, str_y, 0, str);
}

static void display_screensaver(void) {
  ucg_ClearScreen(ucg);
  ucg_SendBuffer(ucg);
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
  display_draw_toggle(50, 2, TOGGLE_WIDTH, TOGGLE_HEIGHT, 0, 0, 0, "S", current_state.hid_led_scroll_lock);
  display_draw_toggle(76, 2, TOGGLE_WIDTH, TOGGLE_HEIGHT, 0, 0, 0, "1", current_state.hid_led_num_lock);
  display_draw_toggle(102, 43, TOGGLE_WIDTH, TOGGLE_HEIGHT, 255, 0, 0, "A", current_state.hid_led_caps_lock);
  ucg_SetFont(ucg, ucg_font_helvB08_hf);
  ucg_SetColor(ucg, 0, 0, 0, 0);
  if(current_state.hid_idle_rate_ms_boot) {
    sprintf(buff, "Idle rate: %dms", current_state.hid_idle_rate_ms_boot);
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

  // Layer Toggles
  ucg_SetFont(ucg, ucg_font_helvB12_hf);
  display_draw_toggle(102, 2, TOGGLE_WIDTH, TOGGLE_HEIGHT, 0, 0, 0, "K", current_state.layer_keypad);
  display_draw_toggle(102, 65, TOGGLE_WIDTH, TOGGLE_HEIGHT, 0, 0, 0, "!", current_state.layer_shifted_number);
  display_draw_toggle(2, 106, TOGGLE_WIDTH, TOGGLE_HEIGHT, 0, 128, 255, "Fn", current_state.layer_fn);

  // Layout Layers
  ucg_SetFont(ucg, ucg_font_helvB12_hf);
  display_draw_toggle(28, 43, 72, TOGGLE_HEIGHT, 0, 0, 0, current_state.layer_keyboard, 1);
  display_draw_toggle(28, 65, 72, TOGGLE_HEIGHT, 64, 64, 64, current_state.layer_computer, 0);

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
  state->hid_idle_rate_ms_boot = hid_idle_rate_ms_boot;
  // TODO move bit logic to hid.c
  state->hid_led_num_lock = hid_led_report & (1<<0);
  state->hid_led_caps_lock = hid_led_report & (1<<1);
  state->hid_led_scroll_lock = hid_led_report & (1<<2);
  state->hid_led_compose = hid_led_report & (1<<3);
  state->hid_led_kana = hid_led_report & (1<<4);

  // Layer toggles
  state->layer_keypad = layers_state[LAYER_KEYPAD];
  state->layer_shifted_number = layers_state[LAYER_SHIFTED_NUMBER];
  state->layer_fn = layers_state[LAYER_FN];

  // Layout layers
  if(layers_state[LAYER_QWERTY_QWERTY]) {
    state->layer_keyboard = "QWERTY";
    state->layer_computer = "QWERTY";
  }
  if(layers_state[LAYER_QWERTY_DVORAK]) {
    state->layer_keyboard = "Dvorak";
    state->layer_computer = "QWERTY";
  }
  if(layers_state[LAYER_DVORAK_DVORAK]) {
    state->layer_keyboard = "Dvorak";
    state->layer_computer = "Dvorak";
  }
  if(layers_state[LAYER_DVORAK_QWERTY]) {
    state->layer_keyboard = "QWERTY";
    state->layer_computer = "Dvorak";
  }

  // Counter
  // state->counter_keys = counter_keys;

  // Right Side
  state->keys_scan_right_side_disconnected = keys_scan_right_side_disconnected;
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
  static uint32_t last_state_change_ms = 0;
  static bool last_screensaver_enabled;

  display_get_current_state(&new_state);
  if(memcmp(&current_state, &new_state, sizeof(struct display_state))) {
    memcpy(&last_state, &current_state, sizeof(struct display_state));
    memcpy(&current_state, &new_state, sizeof(struct display_state));
    last_state_change_ms = uptime_ms();
    display_draw();
    return;
  }

  if(
    (uptime_ms() - last_key_trigger_ms) < (SCREENSAVER_SECS * 1000) ||
    (uptime_ms() - last_state_change_ms) < (SCREENSAVER_SECS * 1000)
  ){
    if(last_screensaver_enabled)
      display_draw();
    last_screensaver_enabled = false;
  } else {
    display_screensaver();
    last_screensaver_enabled = true;
  }
}