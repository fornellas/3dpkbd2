#include "display.h"
#include "hid.h"
#include "usb.h"
#include "lib/systick.h"
#include "lib/images/print_head.h"
#include "lib/images/boot.h"
#include "lib/images/report.h"
#include <stdio.h>
#include <string.h>

#define TOGGLE_WIDTH 24
#define TOGGLE_HEIGHT 20

static ucg_t *ucg;

void display_draw_toggle(ucg_int_t, ucg_int_t, ucg_int_t, ucg_int_t, const char *, uint8_t);

struct display_state {
  uint8_t usbd_state;
  uint8_t usbd_remote_wakeup_enabled;

  uint8_t hid_protocol;
  uint16_t hid_idle_rate_ms;
  uint8_t hid_led_num_lock;
  uint8_t hid_led_caps_lock;
  uint8_t hid_led_scroll_lock;
  uint8_t hid_led_compose;
  uint8_t hid_led_kana;

  // unsigned int layout_keypad;
  // unsigned int layout_shift_lock;
  // unsigned int layout_fn;
  // unsigned int layout_keyboard;
  // unsigned int layout_computer;

  // uint32_t counter_keys;
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

  if(current_state.usbd_state != USBD_STATE_CONFIGURED) {
    usb_draw_display_not_configured(
      current_state.usbd_state,
      current_state.usbd_remote_wakeup_enabled
    );
    ucg_SendBuffer(ucg);
    return;
  }

  ucg_SetColor(ucg, 0, 255, 255, 255);
  ucg_DrawBox(ucg, 0, 0, ucg_GetWidth(ucg), ucg_GetHeight(ucg));

  ucg_DrawPixmap(ucg, 2, 2, print_head_width, print_head_height, print_head_data);

  ucg_SetFont(ucg, ucg_font_helvB12_hf);

  ucg_SetColor(ucg, 0, 0, 0, 0);
  display_draw_toggle(50, 2, TOGGLE_WIDTH, TOGGLE_HEIGHT, "S", current_state.hid_led_scroll_lock);

  ucg_SetColor(ucg, 0, 0, 0, 0);
  display_draw_toggle(76, 2, TOGGLE_WIDTH, TOGGLE_HEIGHT, "1", current_state.hid_led_num_lock);

  // ucg_SetColor(ucg, 0, 0, 0, 0);
  // display_draw_toggle(102, 2, TOGGLE_WIDTH, TOGGLE_HEIGHT, "K", current_state.layout_keypad);

  ucg_SetColor(ucg, 0, 255, 0, 0);
  display_draw_toggle(102, 43, TOGGLE_WIDTH, TOGGLE_HEIGHT, "A", current_state.hid_led_caps_lock);

  // ucg_SetColor(ucg, 0, 0, 0, 0);
  // display_draw_toggle(102, 65, TOGGLE_WIDTH, TOGGLE_HEIGHT, "!", current_state.layout_shift_lock);

  // ucg_SetColor(ucg, 0, 0, 0, 0);
  // display_draw_toggle(2, 106, TOGGLE_WIDTH, TOGGLE_HEIGHT, "Fn", current_state.layout_fn);

  ucg_SetColor(ucg, 0, 0, 0, 0);
  ucg_SetFont(ucg, ucg_font_helvB08_hf);
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

  // state->layout_keypad = layout_keypad;
  // state->layout_shift_lock = layout_shift_lock;
  // state->layout_fn = layout_fn;
  // state->layout_keyboard = layout_keyboard;
  // state->layout_computer = layout_computer;

  // state->counter_keys = counter_keys;
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