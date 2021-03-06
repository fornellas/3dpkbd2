#include "display.h"
#include "dfu.h"
#include "lib/images/UFQFPN48.h"
#include "usb.h"
#include <stdio.h>
#include <string.h>

static ucg_t *ucg;

struct display_state {
	uint8_t usbd_state;
	uint8_t usbd_suspended;

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
	ucg_int_t y_space;

	if(!(current_state.usbd_state == USBD_STATE_CONFIGURED && !current_state.usbd_suspended)) {
		display_draw_usbd_status(current_state.usbd_state, current_state.usbd_suspended, 0);
		ucg_SendBuffer(ucg);
		return;
	}

	ucg_SetColor(ucg, 0, 255, 255, 255);
	ucg_DrawBox(ucg, 0, 0, ucg_GetWidth(ucg), ucg_GetHeight(ucg));


	ucg_SetColor(ucg, 0, 39, 39, 39);
	ucg_SetFont(ucg, ucg_font_helvB18_hr);

	y_space = (ucg_GetHeight(ucg) - ucg_GetFontAscent(ucg) - UFQFPN48_height) / 3;
	y_offset = y_space + ucg_GetFontAscent(ucg);
	ucg_DrawStringCentered(ucg, "DFU", y_offset);

	y_offset += y_space;

	ucg_DrawPixmap(
		ucg,
		(ucg_GetWidth(ucg) - UFQFPN48_width) / 2,
		y_offset,
		UFQFPN48_width,
		UFQFPN48_height,
		UFQFPN48_data
	);

	ucg_SetFont(ucg, ucg_font_helvB12_hr);
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

	ucg_SendBuffer(ucg);
}

static void display_get_current_state(struct display_state *state) {
	state->usbd_state = usbd_state;
	state->usbd_suspended = usbd_is_suspended();

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

void display_power_down(void) {
	ucg_PowerDown(ucg);
}