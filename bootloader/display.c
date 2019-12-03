#include "display.h"
#include "dfu.h"
#include "lib/images/usb.h"
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
	uint8_t force;
} __attribute__((packed));

static struct display_state current_state;
static struct display_state last_state;

static void draw_centered_text(char *, ucg_int_t);

static void draw_centered_text(char *str, ucg_int_t y) {
	ucg_int_t x;

	x = (ucg_GetWidth(ucg) / 2) - (ucg_GetStrWidth(ucg, str) / 2);
	if(!y)
		y = (ucg_GetHeight(ucg) / 2) + (ucg_GetFontAscent(ucg) / 2);
	ucg_DrawString(ucg, x, y, 0, str);
}

static void draw(void) {
	char buff[30];
	ucg_int_t y_offset = 0;

	switch(current_state.usbd_state) {
		case USBD_STATE_RESET:
		case USBD_STATE_SUSPENDED:
		case USBD_STATE_ADDRESSED:
			if(
				current_state.usbd_state != last_state.usbd_state ||
				current_state.force
			) {
				ucg_SetColor(ucg, 0, 255, 255, 255);
				ucg_DrawBox(ucg, 0, 0, ucg_GetWidth(ucg), ucg_GetHeight(ucg));

				ucg_SetColor(ucg, 0, 39, 39, 39);
				ucg_SetScale2x2(ucg);
				y_offset = ucg_GetFontAscent(ucg) + 2;
				draw_centered_text("USB", y_offset);
				ucg_UndoScale(ucg);

				y_offset = y_offset + (ucg_GetHeight(ucg) - usb_height) / 2 + ucg_GetFontAscent(ucg) / 2;

				ucg_DrawPixmap(
					ucg,
					ucg_GetWidth(ucg) - usb_width,
					(ucg_GetHeight(ucg) - usb_height),
					usb_width,
					usb_height,
					usb_data
				);
			}

			switch(current_state.usbd_state) {
				case USBD_STATE_RESET:
					if(
						current_state.usbd_state != last_state.usbd_state ||
						current_state.force
					){
						ucg_SetColor(ucg, 0, 39, 39, 39);
						draw_centered_text("Reset", y_offset);
					}
					break;
				case USBD_STATE_SUSPENDED:
					if(
						current_state.usbd_state != last_state.usbd_state ||
						current_state.force
					){
						ucg_SetColor(ucg, 0, 39, 39, 39);
						draw_centered_text("Suspended", y_offset);
					}
					break;
				case USBD_STATE_ADDRESSED:
					if(
						current_state.usbd_state != last_state.usbd_state ||
						current_state.force
					){
						ucg_SetColor(ucg, 0, 39, 39, 39);
						draw_centered_text("Addressed", y_offset);
					}
					break;
			}
			break;
		case USBD_STATE_CONFIGURED:
			if(
				current_state.usbd_state != last_state.usbd_state ||
				current_state.dfu_state != last_state.dfu_state ||
				current_state.dfu_bytes != last_state.dfu_bytes ||
				current_state.force
			) {
				ucg_SetColor(ucg, 0, 255, 255, 255);
				ucg_DrawBox(ucg, 0, 0, ucg_GetWidth(ucg), ucg_GetHeight(ucg));

				ucg_SetColor(ucg, 0, 39, 39, 39);
				ucg_SetScale2x2(ucg);
				y_offset = ucg_GetFontAscent(ucg) + 2;
				draw_centered_text("DFU", y_offset);
				ucg_UndoScale(ucg);

				y_offset += (ucg_GetHeight(ucg) - UFQFPN48_height ) / 2;

				ucg_DrawPixmap(
					ucg,
					(ucg_GetWidth(ucg) - UFQFPN48_width) / 2,
					y_offset,
					UFQFPN48_width,
					UFQFPN48_height,
					UFQFPN48_data
				);

				y_offset += UFQFPN48_height / 2;

				// sprintf(buff, "dfu_status %d", current_state.dfu_status);
				// draw_centered_text(buff);

				switch(current_state.dfu_state) {
					case STATE_DFU_IDLE:
						break;
					case STATE_DFU_DNLOAD_SYNC:
					case STATE_DFU_DNLOAD_IDLE:
						sprintf(buff, "%ldk", current_state.dfu_bytes / 1024);
						ucg_SetScale2x2(ucg);
						ucg_SetColor(ucg, 0, 255, 255, 255);
						draw_centered_text(buff, y_offset / 2 + ucg_GetFontAscent(ucg) / 2);
						ucg_UndoScale(ucg);
						break;
					case STATE_DFU_MANIFEST_SYNC:
						ucg_SetColor(ucg, 0, 255, 255, 255);
						draw_centered_text("Finished", y_offset + ucg_GetFontAscent(ucg) / 2);
						break;
					case STATE_DFU_ERROR:
						ucg_SetColor(ucg, 0, 255, 255, 255);
						draw_centered_text("Error", y_offset + ucg_GetFontAscent(ucg) / 2);
						break;
					default:
						ucg_SetColor(ucg, 0, 255, 255, 255);
						draw_centered_text("Unknown", y_offset + ucg_GetFontAscent(ucg) / 2);
						break;
				}
			}
			break;
		default:
			ucg_ClearScreen(ucg);
			ucg_SetColor(ucg, 0, 255, 255, 255);
			draw_centered_text("USB Unknown", 0);
			break;
	}
	ucg_SendBuffer(ucg);
}

static void get_new_display_state(struct display_state *state) {
	state->usbd_state = usbd_state;
	state->dfu_status = dfu_status;
	state->dfu_state = dfu_state;
	state->dfu_address = dfu_address;
	state->dfu_bytes = dfu_bytes;
	state->dfu_block_num = dfu_block_num;
	state->force = 0;
}

static void set_display_state(struct display_state *new_state) {
	memcpy(&last_state, &current_state, sizeof(struct display_state));
	memcpy(&current_state, new_state, sizeof(struct display_state));
}

static uint8_t is_different_state(struct display_state *new_state) {
	return memcmp(&current_state, new_state, sizeof(struct display_state));
}

void display_setup(void) {
	ucg = display_setup_base();
}

void display_update(void) {
	static uint8_t first = 1;
	struct display_state new_state;

	get_new_display_state(&new_state);

	if(first) {
		set_display_state(&new_state);
		set_display_state(&new_state);
		current_state.force = 1;

		ucg_SetColor(ucg, 0, 255, 255, 255);
		ucg_ClearScreen(ucg);

		ucg_SetFontMode(ucg, UCG_FONT_MODE_TRANSPARENT);
		ucg_SetFont(ucg, ucg_font_helvB10_hf);

		draw();
		first = 0;
		return;
	}

	if(is_different_state(&new_state)) {
		set_display_state(&new_state);
		draw();
	}
}

