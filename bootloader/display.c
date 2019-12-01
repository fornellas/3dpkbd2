#include "display.h"
#include "dfu.h"
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

void draw_centered_text(char *, ucg_int_t);

void draw_centered_text(char *str, ucg_int_t y) {
	ucg_int_t x;

	x = (ucg_GetWidth(ucg) / 2) - (ucg_GetStrWidth(ucg, str) / 2);
	if(!y)
		y = (ucg_GetHeight(ucg) / 2) + (ucg_GetFontAscent(ucg) / 2);
	ucg_DrawString(ucg, x, y, 0, str);
}

static void draw(void) {
	char buff[30];

	// ucg_ClearScreen(ucg);

	switch(current_state.usbd_state) {
		case USBD_STATE_RESET:
			if(last_state.usbd_state != USBD_STATE_RESET)
				ucg_ClearScreen(ucg);
			draw_centered_text("USB Disconnected", 0);
			break;
		case USBD_STATE_SUSPENDED:
			if(last_state.usbd_state != USBD_STATE_SUSPENDED)
				ucg_ClearScreen(ucg);
			draw_centered_text("USB Suspended", 0);
			break;
		case USBD_STATE_ADDRESSED:
			if(last_state.usbd_state != USBD_STATE_ADDRESSED)
				ucg_ClearScreen(ucg);
			draw_centered_text("USB Addressed", 0);
			break;
		case USBD_STATE_CONFIGURED:
			if(last_state.usbd_state != USBD_STATE_CONFIGURED) {
				ucg_ClearScreen(ucg);
				ucg_SetScale2x2(ucg);
				draw_centered_text("DFU 1.1", ucg_GetFontAscent(ucg));
				ucg_UndoScale(ucg);
			}

			// sprintf(buff, "dfu_status %d", current_state.dfu_status);
			// draw_centered_text(buff);

			switch(current_state.dfu_state) {
				case STATE_DFU_IDLE:
					if(last_state.dfu_state != STATE_DFU_IDLE){
						ucg_SetColor(ucg, 0, 0, 0, 0);
						ucg_DrawBox(ucg, 0, ucg_GetFontAscent(ucg) * 5, ucg_GetWidth(ucg), ucg_GetFontAscent(ucg));
						ucg_SetColor(ucg, 0, 255, 255, 255);
					}
					draw_centered_text("Idle", ucg_GetFontAscent(ucg) * 6);
					break;
				case STATE_DFU_DNLOAD_SYNC:
				case STATE_DFU_DNLOAD_IDLE:
					if(last_state.dfu_state != STATE_DFU_IDLE){
						ucg_SetColor(ucg, 0, 0, 0, 0);
						ucg_DrawBox(ucg, 0, ucg_GetFontAscent(ucg) * 5, ucg_GetWidth(ucg), ucg_GetFontAscent(ucg) * 3);
						ucg_SetColor(ucg, 0, 255, 255, 255);
					}
					draw_centered_text("Downloading", ucg_GetFontAscent(ucg) * 6);
					sprintf(buff, "%ldk", current_state.dfu_bytes / 1024);
					draw_centered_text(buff, ucg_GetFontAscent(ucg) * 8);
					break;
				case STATE_DFU_MANIFEST_SYNC:
					if(last_state.dfu_state != STATE_DFU_IDLE){
						ucg_SetColor(ucg, 0, 0, 0, 0);
						ucg_DrawBox(ucg, 0, ucg_GetFontAscent(ucg) * 5, ucg_GetWidth(ucg), ucg_GetFontAscent(ucg) * 3);
						ucg_SetColor(ucg, 0, 255, 255, 255);
					}
					draw_centered_text("Finished!", ucg_GetFontAscent(ucg) * 6);
					break;
				case STATE_DFU_ERROR:
					if(last_state.dfu_state != STATE_DFU_IDLE){
						ucg_SetColor(ucg, 0, 0, 0, 0);
						ucg_DrawBox(ucg, 0, ucg_GetFontAscent(ucg) * 5, ucg_GetWidth(ucg), ucg_GetFontAscent(ucg) * 3);
						ucg_SetColor(ucg, 0, 255, 255, 255);
					}
					draw_centered_text("Error!", ucg_GetFontAscent(ucg) * 6);
					break;
				default:
					ucg_SetColor(ucg, 0, 0, 0, 0);
					ucg_DrawBox(ucg, 0, ucg_GetFontAscent(ucg) * 5, ucg_GetWidth(ucg), ucg_GetFontAscent(ucg) * 3);
					ucg_SetColor(ucg, 0, 255, 255, 255);
					draw_centered_text("Unknown", ucg_GetFontAscent(ucg) * 6);
					break;
			}
			break;
		default:
			ucg_ClearScreen(ucg);
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
		ucg_SetColor(ucg, 0, 255, 255, 255);
		ucg_SetColor(ucg, 1, 0, 0, 0);
		ucg_ClearScreen(ucg);
		ucg_SetFontMode(ucg, UCG_FONT_MODE_SOLID);
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

