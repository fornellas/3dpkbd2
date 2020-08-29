#ifndef DISPLAY_H
#define DISPLAY_H

#include <ucg.h>

ucg_t *display_setup_base(void);

void display_refresh(void);

void display_draw_usbd_status(uint8_t, uint8_t, uint8_t);

void ucg_DrawPixmap(ucg_t *ucg, ucg_int_t x, ucg_int_t y, ucg_int_t width, ucg_int_t height, const unsigned char *data);

void ucg_DrawStringCentered(ucg_t *, const char *, ucg_int_t);

#endif