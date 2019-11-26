#include "descriptors.h"
#include "dfu.h"
#include "usb.h"
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/dwc/otg_fs.h>

//
// Variables
//

uint8_t usbd_control_buffer[];

uint8_t usbd_state;

//
// Prototypes
//

static void reset_callback(void);

static void suspend_callback(void);

static void resume_callback(void);

static void set_config_callback(usbd_device *, uint16_t);

//
// Functions
//

static void reset_callback() {
	usbd_state = USBD_STATE_RESET;
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

	usbd_state = USBD_STATE_ADDRESSED;

	return USBD_REQ_NEXT_CALLBACK;
}

static void suspend_callback() {
	usbd_state = USBD_STATE_SUSPENDED;
}

static void resume_callback() {
	usbd_state = USBD_STATE_RESET;
}

static void set_config_callback(usbd_device *dev, uint16_t wValue) {
	if(wValue != CONFIGURATION_VALUE)
		return;

	usbd_state = USBD_STATE_CONFIGURED;

	dfu_set_config_callback(dev);
}

usbd_device *usbd_setup() {
	usbd_device *usbd_dev;

	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

	desig_get_unique_id_as_string(usb_serial_number, sizeof(usb_serial_number));

	reset_callback();

	usbd_dev = usbd_init(
		&otgfs_usb_driver,
		&dev_descr,
		&conf_descr,
		usb_strings, 4,
		usbd_control_buffer,
		sizeof(usbd_control_buffer)
	);

	usbd_state = USBD_STATE_RESET;

	usbd_register_reset_callback(usbd_dev, reset_callback);

	usbd_register_control_callback(
		usbd_dev,
		USB_REQ_TYPE_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		set_address_callback
	);

	usbd_register_set_config_callback(usbd_dev, set_config_callback);

	usbd_register_suspend_callback(usbd_dev, suspend_callback);

	usbd_register_resume_callback(usbd_dev, resume_callback);


	// https://github.com/libopencm3/libopencm3/issues/1119#issuecomment-549071405
	// Fix for otgfs_usb_driver setting VBUSBSEN regardless of what board it is.
	OTG_FS_GCCFG |= OTG_GCCFG_NOVBUSSENS;

	return usbd_dev;
}