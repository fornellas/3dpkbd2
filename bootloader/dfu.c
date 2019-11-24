#include "dfu.h"
#include "usb.h"
#include "descriptors.h"
#include <libopencm3/usb/dfu.h>
#include <string.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/scb.h>

/* Commands sent with wBlockNum == 0 as per ST implementation. */
#define CMD_SETADDR	0x21
#define CMD_ERASE	0x41

static enum dfu_state dfu_state = STATE_DFU_IDLE;

static struct {
	uint8_t buf[sizeof(usbd_control_buffer)];
	uint16_t len;
	uint32_t addr;
	uint16_t blocknum;
} prog;

static uint8_t dfu_getstatus(uint32_t * bwPollTimeout) {
	switch(dfu_state) {
		case STATE_DFU_DNLOAD_SYNC:
			dfu_state = STATE_DFU_DNBUSY;
			*bwPollTimeout = 100;
			return DFU_STATUS_OK;
		case STATE_DFU_MANIFEST_SYNC:
			/* Device will reset when read is complete. */
			dfu_state = STATE_DFU_MANIFEST;
			return DFU_STATUS_OK;
		default:
			return DFU_STATUS_OK;
		}
}

static uint8_t get_flash_sector(uint32_t address) {
	if(address >= 0x08000000 && address <= 0x08003FFF)
		return 0;
	if(address >= 0x08004000 && address <= 0x08007FFF)
		return 1;
	if(address >= 0x08008000 && address <= 0x0800BFFF)
		return 2;
	if(address >= 0x0800C000 && address <= 0x0800FFFF)
		return 3;
	if(address >= 0x08010000 && address <= 0x0801FFFF)
		return 4;
	if(address >= 0x08020000 && address <= 0x0803FFFF)
		return 5;
	if(address >= 0x08040000 && address <= 0x0805FFFF)
		return 6;
	if(address >= 0x08060000 && address <= 0x0807FFFF)
		return 7;
	return 7;
}

static void dfu_getstatus_complete(
	usbd_device * usbd_dev,
	struct usb_setup_data *req
) {
	int i;
	(void) req;
	(void) usbd_dev;

	switch(dfu_state) {
		case STATE_DFU_DNBUSY:
			flash_unlock();
			if(prog.blocknum == 0) {
				switch(prog.buf[0]) {
					case CMD_ERASE:
						{
							uint32_t *dat =(uint32_t *)(prog.buf + 1);
							uint8_t sector = get_flash_sector(*dat);
							flash_erase_sector(sector, 3);
						}
					case CMD_SETADDR:
						{
							uint32_t *dat =(uint32_t *)(prog.buf + 1);
							prog.addr = *dat;
						}
				}
			} else {
				uint32_t baseaddr = prog.addr +((prog.blocknum - 2) * dfu_function.wTransferSize);
				for(i = 0; i < prog.len; i += 2) {
						uint16_t *dat =(uint16_t *)(prog.buf + i);
						flash_program_half_word(baseaddr + i, *dat);
					}
			}
			flash_lock();

			/* Jump straight to dfuDNLOAD-IDLE, skipping dfuDNLOAD-SYNC. */
			dfu_state = STATE_DFU_DNLOAD_IDLE;
			return;
		case STATE_DFU_MANIFEST:
			/* USB device must detach, we just reset... */
			scb_reset_system();
			return;			/* Will never return. */
		default:
			return;
		}
}

static enum usbd_request_return_codes dfu_control_request(
	usbd_device * usbd_dev,
	struct usb_setup_data *req,
	uint8_t ** buf,
	uint16_t * len,
	void(**complete)(usbd_device * usbd_dev, struct usb_setup_data * req)
) {
	(void) usbd_dev;

	if((req->bmRequestType & 0x7F) != 0x21)
		return USBD_REQ_NOTSUPP;	/* Only accept class request. */

	switch(req->bRequest) {
		case DFU_DNLOAD:
			if((len == NULL) ||(*len == 0)) {
				dfu_state = STATE_DFU_MANIFEST_SYNC;
			} else {
				/* Copy download data for use on GET_STATUS. */
				prog.blocknum = req->wValue;
				prog.len = *len;
				memcpy(prog.buf, *buf, *len);
				dfu_state = STATE_DFU_DNLOAD_SYNC;
			}
			return USBD_REQ_HANDLED;
		case DFU_CLRSTATUS:
			/* Clear error and return to dfuIDLE. */
			if(dfu_state == STATE_DFU_ERROR)
				dfu_state = STATE_DFU_IDLE;
			return USBD_REQ_HANDLED;
		case DFU_ABORT:
			/* Abort returns to dfuIDLE state. */
			dfu_state = STATE_DFU_IDLE;
			return USBD_REQ_HANDLED;
		case DFU_UPLOAD:
			/* Upload not supported for now. */
			return USBD_REQ_NOTSUPP;
		case DFU_GETSTATUS:
			{
				uint32_t bwPollTimeout = 0;	/* 24-bit integer in DFU class spec */

				(*buf)[0] = dfu_getstatus(&bwPollTimeout);
				(*buf)[1] = bwPollTimeout & 0xFF;
				(*buf)[2] =(bwPollTimeout >> 8) & 0xFF;
				(*buf)[3] =(bwPollTimeout >> 16) & 0xFF;
				(*buf)[4] = dfu_state;
				(*buf)[5] = 0;		/* iString not used here */
				*len = 6;
				*complete = dfu_getstatus_complete;
				return USBD_REQ_HANDLED;
			}
		case DFU_GETSTATE:
			/* Return state with no state transision. */
			*buf[0] = dfu_state;
			*len = 1;
			return USBD_REQ_HANDLED;
	}

	return USBD_REQ_NOTSUPP;
}

void
dfu_set_config_callback(usbd_device * usb_dev) {
	usbd_register_control_callback(
		usb_dev,
		USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
		USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		dfu_control_request
	);
}
