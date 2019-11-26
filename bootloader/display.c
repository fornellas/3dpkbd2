#include "display.h"
#include "usb.h"
#include <stdio.h>

static uint8_t last_usbd_state;

static void draw(void) {
	char *buff = NULL;

	switch(last_usbd_state) {
		case USBD_STATE_RESET:
			buff = "USB RESET";
			break;
		case USBD_STATE_ADDRESSED:
			buff = "USB ADDRESSED";
			break;
		case USBD_STATE_CONFIGURED:
			buff = "USB CONFIGURED";
			break;
		case USBD_STATE_SUSPENDED:
			buff = "USB SUSPENDED";
			break;
	}

	ucg_SetColor(&ucg, 0, 255, 255, 255);
	ucg_SetColor(&ucg, 1, 0, 0, 0);
	ucg_SetFontMode(&ucg, UCG_FONT_MODE_SOLID);
	ucg_SetFont(&ucg, ucg_font_ncenR10_tr);
	ucg_ClearScreen(&ucg);
	ucg_DrawString(&ucg, 5, 15 + 15 * last_usbd_state, 0, buff);
}

void display_update(void) {
	static uint8_t first = 1;

	if(first) {
		last_usbd_state = usbd_state;
		draw();
		first = 0;
		return;
	}

	if(usbd_state != last_usbd_state) {
		last_usbd_state = usbd_state;
		draw();
	}
}