#include "display.h"
#include "hid.h"
#include "usb.h"
#include "lib/systick.h"
#include <stdio.h>
#include <string.h>

static ucg_t *ucg;

struct display_state {
  uint8_t usbd_state;
  uint8_t hid_protocol;
  int16_t hid_idle_rate_ms;
  hid_out_report_data hid_led_report;
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
  state->hid_protocol = hid_protocol;
  state->hid_idle_rate_ms = hid_idle_rate_ms;
  state->hid_led_report = hid_led_report;
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