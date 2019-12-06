#include <ucg.h>

ucg_t *display_setup_base(void);

void display_refresh(void);

void usb_draw_display_not_configured(uint8_t _usbd_state);

void ucg_DrawPixmap(ucg_t *ucg, ucg_int_t x, ucg_int_t y, ucg_int_t width, ucg_int_t height, const unsigned char *data);

void ucg_DrawStringCentered(ucg_t *, const char *, ucg_int_t);