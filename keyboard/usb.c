#include "usb.h"
#include "hid.h"
#include <libopencm3/stm32/desig.h>

//
// Variables
//

uint8_t usbd_control_buffer[];
uint8_t usb_remote_wakeup_enabled = 0;

//
// Prototypes
//

void set_config_callback(usbd_device *dev, uint16_t wValue);

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

	set_config_callback_base(dev, wValue);

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
}

usbd_device *usbd_setup() {
	usbd_device *usbd_dev;

	desig_get_unique_id_as_string(usb_serial_number, sizeof(usb_serial_number));

	usbd_dev = usbd_setup_base(
		&dev_descr,
		&conf_descr,
		usb_strings,
		USB_STRINGS_NUM,
		usbd_control_buffer,
		sizeof(usbd_control_buffer)
	);

	usbd_register_set_config_callback(usbd_dev, set_config_callback);

	return usbd_dev;
}