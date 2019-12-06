#include "display.h"
#include "dfu.h"
#include "lib/images/UFQFPN48.h"
#include "usb.h"
#include <stdio.h>
#include <string.h>

static ucg_t *ucg;

struct display_state {
	uint8_t usbd_state;
	uint8_t dfu_status;
	uint8_t dfu_state;
	uint32_t dfu_address;
	uint32_t dfu_bytes;
	uint16_t dfu_block_num;
} __attribute__((packed));

static struct display_state current_state;
static struct display_state last_state;

static void display_draw(void) {
	char buff[30];
	ucg_int_t y_offset = 0;

	switch(current_state.usbd_state) {
		case USBD_STATE_RESET:
		case USBD_STATE_SUSPENDED:
		case USBD_STATE_ADDRESSED:
			usb_draw_display_not_configured(current_state.usbd_state);
			break;
		case USBD_STATE_CONFIGURED:
			ucg_SetColor(ucg, 0, 255, 255, 255);
			ucg_DrawBox(ucg, 0, 0, ucg_GetWidth(ucg), ucg_GetHeight(ucg));

			ucg_SetColor(ucg, 0, 39, 39, 39);
			ucg_SetFont(ucg, ucg_font_helvB24_hf);
			y_offset = ucg_GetFontAscent(ucg) + 2;
			ucg_DrawStringCentered(ucg, "DFU", y_offset);

			y_offset += (ucg_GetHeight(ucg) - y_offset - UFQFPN48_height) / 2;

			ucg_DrawPixmap(
				ucg,
				(ucg_GetWidth(ucg) - UFQFPN48_width) / 2,
				y_offset,
				UFQFPN48_width,
				UFQFPN48_height,
				UFQFPN48_data
			);

			ucg_SetFont(ucg, ucg_font_helvB14_hf);
			y_offset += UFQFPN48_height / 2 + ucg_GetFontAscent(ucg) / 2 - 6;
			ucg_SetColor(ucg, 0, 255, 255, 255);

			if(current_state.dfu_status == DFU_STATUS_OK) {
				switch(current_state.dfu_state) {
					case STATE_DFU_IDLE:
						break;
					case STATE_DFU_DNLOAD_SYNC:
					case STATE_DFU_DNLOAD_IDLE:
						sprintf(buff, "%ldkb", current_state.dfu_bytes / 1024);
						ucg_DrawStringCentered(ucg, buff, y_offset);
						break;
					case STATE_DFU_MANIFEST_SYNC:
						ucg_SetColor(ucg, 0, 0, 255, 0);
						ucg_DrawStringCentered(ucg, "Complete", y_offset);
						break;
					case STATE_DFU_ERROR:
						ucg_SetColor(ucg, 0, 255, 0, 0);
						ucg_DrawStringCentered(ucg, "Error!", y_offset);
						break;
					default:
						ucg_SetColor(ucg, 0, 255, 0, 0);
						ucg_DrawStringCentered(ucg, "Unknown", y_offset);
						break;
				}
			} else {
				ucg_DrawStringCentered(ucg, "Error!", y_offset);
			}
			break;
		default:
			ucg_ClearScreen(ucg);
			ucg_SetColor(ucg, 0, 255, 0, 0);
			ucg_SetFont(ucg, ucg_font_helvB14_hf);
			ucg_DrawStringCentered(ucg, "USB Unknown", 0);
			break;
	}
	ucg_SendBuffer(ucg);
}

static void display_get_current_state(struct display_state *state) {
	state->usbd_state = usbd_state;
	state->dfu_status = dfu_status;
	state->dfu_state = dfu_state;
	state->dfu_address = dfu_address;
	state->dfu_bytes = dfu_bytes;
	state->dfu_block_num = dfu_block_num;
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