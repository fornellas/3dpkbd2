#include "display.h"
#include "hid.h"
#include "usb.h"
#include "lib/systick.h"
#include <stdio.h>
#include <string.h>

static ucg_t *ucg;

struct display_state {
  uint8_t usbd_state;
  uint8_t usbd_remote_wakeup_enabled;

  uint8_t hid_protocol;
  int16_t hid_idle_rate_ms;
  unsigned int hid_led_num_lock : 1;
  unsigned int hid_led_caps_lock : 1;
  unsigned int hid_led_scroll_lock : 1;
  unsigned int hid_led_compose : 1;
  unsigned int hid_led_kana : 1;

  // unsigned int layout_keypad : 1;
  // unsigned int layout_shift_lock : 1;
  // unsigned int layout_fn : 1;
  // unsigned int layout_keyboard : 2;
  // unsigned int layout_computer : 2;

  // uint32_t counter_keys : 0;
} __attribute__((packed));

static struct display_state current_state;
static struct display_state last_state;

static void display_draw(void) {
  if(current_state.usbd_state != USBD_STATE_CONFIGURED) {
    usb_draw_display_not_configured(current_state.usbd_state);
    ucg_SendBuffer(ucg);
    return;
  }

  ucg_SetColor(ucg, 0, 255, 0, 0);
  ucg_SetColor(ucg, 2, 0, 255, 0);
  ucg_SetColor(ucg, 1, 0, 0, 255);
  ucg_SetColor(ucg, 3, 255, 255, 255);
  ucg_DrawGradientBox(ucg, 0, 0, ucg_GetWidth(ucg), ucg_GetHeight(ucg));
  ucg_SendBuffer(ucg);
}

static void display_get_current_state(struct display_state *state) {
  state->usbd_state = usbd_state;
  state->usbd_remote_wakeup_enabled = usbd_remote_wakeup_enabled;

  state->hid_protocol = hid_protocol;
  state->hid_idle_rate_ms = hid_idle_rate_ms;
  state->hid_led_num_lock = hid_led_report & (1<<0);
  state->hid_led_caps_lock = hid_led_report & (1<<1);
  state->hid_led_scroll_lock = hid_led_report & (1<<2);
  state->hid_led_compose = hid_led_report & (1<<3);
  state->hid_led_kana = hid_led_report & (1<<4);
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