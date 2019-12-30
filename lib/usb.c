#include "usb.h"
#include "lib/systick.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/dwc/otg_fs.h>

//
// Variables
//

uint8_t usbd_state;
uint8_t usbd_suspended;

//
// Prototypes
//

static void reset_callback(void);

static void suspend_callback(void);

static void resume_callback(void);

//
// Functions
//

static void set_state(uint8_t state) {
	usbd_state = state;
	usbd_suspended = 0;
}

static void reset_callback() {
	set_state(USBD_STATE_RESET);
}

static enum usbd_request_return_codes set_address_callback(
	usbd_device * usbd_dev,
	struct usb_setup_data *req,
	uint8_t ** buf,
	uint16_t * len,
	void(**complete)(usbd_device * usbd_dev, struct usb_setup_data * req)
) {
	(void)usbd_dev;
	(void)req;
	(void)buf;
	(void)len;
	(void)complete;

	set_state(USBD_STATE_ADDRESSED);

	return USBD_REQ_NEXT_CALLBACK;
}

static void suspend_callback() {
	usbd_suspended = 1;
}

static void resume_callback() {
	usbd_suspended = 0;
}

#ifdef USBD_REMOTE_WAKEUP

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
		if(usbd_remote_wakeup_enabled)
			(*buf)[0] |= USB_DEV_STATUS_REMOTE_WAKEUP;
		(*buf)[1] = 0;
		return USBD_REQ_HANDLED;
	}

	return USBD_REQ_NEXT_CALLBACK;
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
				usbd_remote_wakeup_enabled = 0;
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
				usbd_remote_wakeup_enabled = 1;
				return USBD_REQ_HANDLED;
			// case USB_FEAT_TEST_MODE:
			// 	return USBD_REQ_HANDLED;
		}
	}

	return USBD_REQ_NEXT_CALLBACK;
}

#endif

void set_config_callback_base(usbd_device *dev, uint16_t wValue) {
	(void)dev;
	(void)wValue;

	set_state(USBD_STATE_CONFIGURED);

	#ifdef USBD_REMOTE_WAKEUP
	usbd_register_control_callback(
		dev,
		USB_REQ_TYPE_IN | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		control_device_get_status_callback
	);

	usbd_remote_wakeup_enabled = 0;
	usbd_register_control_callback(
		dev,
		USB_REQ_TYPE_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		control_device_feature_callback
	);
	#endif
}

usbd_device *usbd_setup_base(
	const struct usb_device_descriptor *dev,
	const struct usb_config_descriptor *conf,
	const char *const *strings,
	int num_strings,
	uint8_t *control_buffer,
	uint16_t control_buffer_size
) {
	usbd_device *usbd_dev;

	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

	reset_callback();

	usbd_dev = usbd_init(
		&otgfs_usb_driver,
		dev,
		conf,
		strings,
		num_strings,
		control_buffer,
		control_buffer_size
	);

	set_state(USBD_STATE_RESET);

	usbd_register_reset_callback(usbd_dev, reset_callback);

	usbd_register_control_callback(
		usbd_dev,
		USB_REQ_TYPE_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		set_address_callback
	);

	usbd_register_suspend_callback(usbd_dev, suspend_callback);

	usbd_register_resume_callback(usbd_dev, resume_callback);

	// https://github.com/libopencm3/libopencm3/issues/1119#issuecomment-549071405
	// Fix for otgfs_usb_driver setting VBUSBSEN regardless of what board it is.
	OTG_FS_GCCFG |= OTG_GCCFG_NOVBUSSENS;

	return usbd_dev;
}

#ifdef USBD_REMOTE_WAKEUP

void usdb_remote_wakeup_signal() {
	uint32_t start_ms;

	start_ms = uptime_ms();
	OTG_FS_DCTL |= OTG_DCTL_RWUSIG;
	while(uptime_ms() - start_ms < 2);
	OTG_FS_DCTL &= ~OTG_DCTL_RWUSIG;

	usbd_suspended = 0;
 }

 #endif