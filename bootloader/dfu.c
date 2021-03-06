#include "descriptors.h"
#include "dfu.h"
#include "display.h"
#include "led.h"
#include "lib/addresses.h"
#include "usb.h"
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/usb/dfu.h>
#include <string.h>

struct dfu_getstatus_payload {
	// An indication of the status resulting from the execution of the most
	// recent request.
	uint8_t bStatus;
	// Minimum time, in milliseconds, that the host should wait before sending
	// a subsequent DFU_GETSTATUS request.
	uint8_t bwPollTimeout[3];
	// An indication of the state that the device is going to enter immediately
	// following transmission of this response. (By the time the host receives
	// this information, this is the current state of the device.)
	uint8_t bState;
	// Index of status description in string table.
	uint8_t iString;
} __attribute__((packed));

uint8_t dfu_status = DFU_STATUS_OK;
uint8_t dfu_state = STATE_DFU_IDLE;
uint32_t dfu_address = MAIN_PROGRAM_BASE;
uint32_t dfu_bytes = 0;
uint16_t dfu_block_num = 0;

static uint32_t erased_sectors = 0;

static void dfu_reset(void) {
	dfu_status = DFU_STATUS_OK;
	dfu_state = STATE_DFU_IDLE;
	dfu_address = MAIN_PROGRAM_BASE;
	dfu_bytes = 0;
	dfu_block_num = 0;
	erased_sectors = 0;
}

static uint8_t get_sector(uint32_t addr) {
	if(addr >= 0x08000000 && addr <= 0x08003FFF)
		return 0;
	if(addr >= 0x08004000 && addr <= 0x08007FFF)
		return 1;
	if(addr >= 0x08008000 && addr <= 0x0800BFFF)
		return 2;
	if(addr >= 0x0800C000 && addr <= 0x0800FFFF)
		return 3;
	if(addr >= 0x08010000 && addr <= 0x0801FFFF)
		return 4;
	if(addr >= 0x08020000 && addr <= 0x0803FFFF)
		return 5;
	if(addr >= 0x08040000 && addr <= 0x0805FFFF)
		return 6;
	if(addr >= 0x08060000 && addr <= 0x0807FFFF)
		return 7;
	return 0;
}

void dfu_write(uint8_t *, uint32_t);

void dfu_write(uint8_t *data, uint32_t length) {
	uint8_t sector;

	led_toggle();

	flash_unlock();

	sector = get_sector(dfu_address);

	if(! ((1<<sector) & erased_sectors)) {
		flash_erase_sector(sector, FLASH_CR_PROGRAM_X8);
		erased_sectors |= (1<<sector);
	}

	flash_program(dfu_address, data, length);
	dfu_address += length;

	flash_lock();
}

static enum usbd_request_return_codes dfu_control_request(
	usbd_device * usbd_dev,
	struct usb_setup_data *req,
	uint8_t ** buf,
	uint16_t * len,
	void(**complete)(usbd_device * usbd_dev, struct usb_setup_data * req)
) {
	uint8_t interface_number;

	(void)usbd_dev;
	(void)complete;

	interface_number = req->wIndex;
	if (interface_number != DFU_INTERFACE_NUMBER)
		return USBD_REQ_NOTSUPP;

	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
		&& (req->bRequest == DFU_DNLOAD)
	) {

		if(req->wLength) {
			if(req->wLength > dfu_function.wTransferSize) {
				dfu_state = STATE_DFU_ERROR;
				dfu_status = DFU_STATUS_ERR_UNKNOWN;
				return USBD_REQ_NOTSUPP;
			}

			if(dfu_address + req->wLength > MAIN_MEMORY_MAX) {
				dfu_state = STATE_DFU_ERROR;
				dfu_status = DFU_STATUS_ERR_ADDRESS;
				return USBD_REQ_HANDLED;
			}

			dfu_status = DFU_STATUS_OK;
			dfu_state = STATE_DFU_DNLOAD_SYNC;

			if(!dfu_bytes)
				display_update();

			dfu_write(*buf, req->wLength);

			dfu_block_num = req->wValue;
			dfu_bytes += req->wLength;

			return USBD_REQ_HANDLED;
		} else {
			dfu_status = DFU_STATUS_OK;
			dfu_state = STATE_DFU_MANIFEST_SYNC;
			return USBD_REQ_HANDLED;
		}
	}

	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == DFU_UPLOAD)
	) {
		// uint32_t wLength;
		// uint8_t *data;

		// wLength = req->wLength;
		// data = *buf;

		// // TODO read 

		// return USBD_REQ_HANDLED;
	}

	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == DFU_GETSTATUS)
	) {
		struct dfu_getstatus_payload *status_payload;

		if(req->wLength != sizeof(struct dfu_getstatus_payload))
			return USBD_REQ_NOTSUPP;

		status_payload = (struct dfu_getstatus_payload *)(*buf);

		status_payload->bStatus = dfu_status;
		status_payload->bwPollTimeout[0] = 0;
		status_payload->bwPollTimeout[1] = 0;
		status_payload->bwPollTimeout[2] = 0;
		if(dfu_state == STATE_DFU_DNLOAD_SYNC) {
			dfu_state = STATE_DFU_DNLOAD_IDLE;
		} else if(dfu_state == STATE_DFU_MANIFEST_SYNC) {
			dfu_reset();
		}
		status_payload->bState = dfu_state;
		status_payload->iString = 0;

		*len = sizeof(struct dfu_getstatus_payload);

		return USBD_REQ_HANDLED;
	}

	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
		&& (req->bRequest == DFU_CLRSTATUS)
	) {
		if (dfu_state == STATE_DFU_ERROR) {
			dfu_reset();
		}
		return USBD_REQ_HANDLED;
	}

	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
		&& (req->bRequest == DFU_GETSTATE)
	) {
		*buf[0] = dfu_state;
		*len = 1;
		return USBD_REQ_HANDLED;
	}

	if(
		((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
		&& (req->bRequest == DFU_ABORT)
	) {
		dfu_reset();
		return USBD_REQ_HANDLED;
	}

	return USBD_REQ_NEXT_CALLBACK;
}

void reset_callback(void);

void reset_callback(void) {
	display_power_down();
	scb_reset_system();
}


void dfu_set_config_callback(usbd_device *usbd_dev) {
	dfu_reset();

	usbd_register_control_callback(
		usbd_dev,
		USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
		USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		dfu_control_request
	);

	usbd_register_reset_callback(usbd_dev, reset_callback);
}
