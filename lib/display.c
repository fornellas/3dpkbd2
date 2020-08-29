#include "display.h"
#include "lib/display_hal.h"
#include "usb.h"
#include "lib/images/usb.h"
#include "lib/systick.h"

static ucg_t ucg;

ucg_t *display_setup_base(void) {
  // Sleep a bit to allow the voltage regulator to stabilize
  delay_ms(50);

  ucg_InitBuffer(
    &ucg,
    ucg_dev_ssd1351_18x128x128_ilsoft,
    ucg_ext_ssd1351_18,
    ucg_com_cm3_4wire_HW_SPI
  );
  ucg.com_cb_funcs.send_byte = ucg_com_cm3_4wire_HW_SPI_send_byte;
  ucg_SendBuffer(&ucg);

  return &ucg;
}

void display_draw_usbd_status(uint8_t state, uint8_t suspended, uint8_t remote_wakeup_enabled) {
  ucg_int_t y_offset;

  ucg_SetColor(&ucg, 0, 255, 255, 255);
  ucg_DrawBox(&ucg, 0, 0, ucg_GetWidth(&ucg), ucg_GetHeight(&ucg));

  ucg_DrawPixmap(
    &ucg,
    ucg_GetWidth(&ucg) - usb_width,
    (ucg_GetHeight(&ucg) - usb_height),
    usb_width,
    usb_height,
    usb_data
  );

  ucg_SetColor(&ucg, 0, 39, 39, 39);
  ucg_SetFont(&ucg, ucg_font_helvB18_hr);
  y_offset = 1 + ucg_GetFontAscent(&ucg);
  ucg_DrawStringCentered(&ucg, "USB", y_offset);

  ucg_SetFont(&ucg, ucg_font_helvB10_hr);
  y_offset += 1 + ucg_GetFontAscent(&ucg);

  switch(state) {
    case USBD_STATE_RESET:
      ucg_SetColor(&ucg, 0, 128, 128, 128);
      ucg_DrawStringCentered(&ucg, "reset", y_offset);
      break;
    case USBD_STATE_ADDRESSED:
      ucg_SetColor(&ucg, 0, 128, 255, 128);
      ucg_DrawStringCentered(&ucg, "addressed", y_offset);
      break;
    case USBD_STATE_CONFIGURED:
      ucg_SetColor(&ucg, 0, 39, 39, 39);
      ucg_DrawStringCentered(&ucg, "configured", y_offset);
      break;
  }

  if(suspended) {
    y_offset += 1 - ucg_GetFontDescent(&ucg) + ucg_GetFontAscent(&ucg);
    ucg_SetColor(&ucg, 0, 128, 128, 255);
    ucg_DrawStringCentered(&ucg, "suspended", y_offset);

    #ifdef USBD_REMOTE_WAKEUP
    if(remote_wakeup_enabled) {
      y_offset += 1 - ucg_GetFontDescent(&ucg);
      ucg_SetFont(&ucg, ucg_font_helvB08_hr);
      y_offset += ucg_GetFontAscent(&ucg);
      ucg_SetColor(&ucg, 0, 0, 128, 0);
      ucg_DrawStringCentered(&ucg, "remote wakeup", y_offset);
    }
    #else
    (void)remote_wakeup_enabled;
    #endif
  }
}

void ucg_DrawPixmap(ucg_t *_ucg, ucg_int_t x, ucg_int_t y, ucg_int_t width, ucg_int_t height, const unsigned char *data) {
  for(ucg_int_t k=0 ; k < height ; k++){
    const unsigned char *row;
    row = data + (k * width * 3);
    for(ucg_int_t j=0 ; j < width ; j++){
      const unsigned char *pixel;
      pixel = row + (j * 3);
      ucg_SetColor(_ucg, 0, *pixel, *(pixel + 1), *(pixel + 2));
      ucg_DrawPixel(_ucg, x + j, y + k);
    }
  }
}

void ucg_DrawStringCentered(ucg_t *_ucg, const char *str, ucg_int_t y) {
  ucg_int_t x;

  x = (ucg_GetWidth(_ucg) / 2) - (ucg_GetStrWidth(_ucg, str) / 2);
  if(!y)
    y = (ucg_GetHeight(_ucg) / 2) + (ucg_GetFontAscent(_ucg) / 2);
  ucg_DrawString(_ucg, x, y, 0, str);
}