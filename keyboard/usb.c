#include "descriptors.h"
#include "hid.h"
#include "usb.h"
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/dwc/otg_fs.h>

//
// Variables
//

extern volatile uint32_t uptime_ms;

uint8_t usb_remote_wakeup_enabled = 0;
uint8_t usb_suspended=0;
// Must be big enough to support HID in reports
uint8_t usbd_control_buffer[128];

//
// Prototypes
//

void set_config_callback(usbd_device *dev, uint16_t wValue);

void reset_callback(void);

void resume_callback(void);

void suspend_callback(void);

//
// Functions
//

static enum usbd_request_return_codes control_device_get_status_callback(
	usbd_device *dev,
	struct usb_setup_data *req,
	uint8_t **buf,
	uint16_t *len,
	void (**complete)(usbd_device *, struct usb_setup_data * )
) {
	(void)dev;
	(void)complete;

	// 9.4.5 Get Status
	if(req->bRequest == USB_REQ_GET_STATUS) {
		*len = 2;
		(*buf)[0] = USB_DEV_STATUS_SELF_POWERED;
		if(usb_remote_wakeup_enabled)
			(*buf)[0] |= USB_DEV_STATUS_REMOTE_WAKEUP;
		(*buf)[1] = 0;
		return USBD_REQ_HANDLED;
	}

	return USBD_REQ_NOTSUPP;
}

static enum usbd_request_return_codes control_device_feature_callback(
	usbd_device *dev,
	struct usb_setup_data *req,
	uint8_t **buf,
	uint16_t *len,
	void (**complete)(usbd_device *, struct usb_setup_data * )
) {
	(void)dev;
	(void)buf;
	(void)len;
	(void)complete;

	// 9.4.1 Clear Feature
	if(req->bRequest == USB_REQ_CLEAR_FEATURE) {
		switch (req->wValue) {
			// case USB_FEAT_ENDPOINT_HALT:
			// 	return USBD_REQ_HANDLED;
			case USB_FEAT_DEVICE_REMOTE_WAKEUP:
				usb_remote_wakeup_enabled = 0;
				return USBD_REQ_HANDLED;
			// case USB_FEAT_TEST_MODE:
			// 	return USBD_REQ_HANDLED;
		}
	}

	// 9.4.9 Set Feature
	if(req->bRequest == USB_REQ_SET_FEATURE) {
		switch (req->wValue) {
			// case USB_FEAT_ENDPOINT_HALT:
			// 	return USBD_REQ_HANDLED;
			case USB_FEAT_DEVICE_REMOTE_WAKEUP:
				usb_remote_wakeup_enabled = 1;
				return USBD_REQ_HANDLED;
			// case USB_FEAT_TEST_MODE:
			// 	return USBD_REQ_HANDLED;
		}
	}

	return USBD_REQ_NOTSUPP;
}

void set_config_callback(usbd_device *dev, uint16_t wValue) {
	if(wValue != CONFIGURATION_VALUE)
		return;

	usbd_register_control_callback(
		dev,
		USB_REQ_TYPE_IN | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		control_device_get_status_callback
	);

	usb_remote_wakeup_enabled = 0;
	usbd_register_control_callback(
		dev,
		USB_REQ_TYPE_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		control_device_feature_callback
	);

	hid_set_config_callback(dev);

	usb_suspended = 0;
}

void reset_callback() {
	hid_poll_disable();
	usb_suspended = 0;
}

void resume_callback() {
	usb_suspended = 0;
}

void suspend_callback() {
	usb_suspended = 1;
}

usbd_device *usbd_setup() {
	usbd_device *usbd_dev;

	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

	desig_get_unique_id_as_string(usb_serial_number, sizeof(usb_serial_number));

	usbd_dev = usbd_init(
		&otgfs_usb_driver,
		&dev_descr,
		&conf_descr,
		usb_strings, 3,
		usbd_control_buffer,
		sizeof(usbd_control_buffer)
	);

	// https://github.com/libopencm3/libopencm3/issues/1119#issuecomment-549071405
	// Fix for otgfs_usb_driver setting VBUSBSEN regardless of what board it is.
	OTG_FS_GCCFG |= OTG_GCCFG_NOVBUSSENS;

	usbd_register_set_config_callback(usbd_dev, set_config_callback);

	usbd_register_reset_callback(usbd_dev, reset_callback);

	usbd_register_resume_callback(usbd_dev, resume_callback);

	// TODO
	// usbd_register_set_altsetting_callback();

	// TODO
	// usbd_register_sof_callback();

	usbd_register_suspend_callback(usbd_dev, suspend_callback);

	return usbd_dev;
}

void usdb_remote_wakeup_signal() {
	uint32_t start_ms = uptime_ms;

	OTG_FS_DCTL |= OTG_DCTL_RWUSIG;
	while(uptime_ms - start_ms < 2);
	OTG_FS_DCTL &= ~OTG_DCTL_RWUSIG;
}