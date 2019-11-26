#include "usb.h"
#include "lib/systick.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/dwc/otg_fs.h>

//
// Variables
//

uint8_t usbd_state;

//
// Prototypes
//

static void reset_callback(void);

static void suspend_callback(void);

static void resume_callback(void);

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

void set_config_callback_base(usbd_device *dev, uint16_t wValue) {
	(void)dev;
	(void)wValue;

	usbd_state = USBD_STATE_CONFIGURED;
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

	usbd_state = USBD_STATE_RESET;

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

void usdb_remote_wakeup_signal() {
       uint32_t start_ms;

       start_ms = uptime_ms();
       OTG_FS_DCTL |= OTG_DCTL_RWUSIG;
       while(uptime_ms() - start_ms < 2);
       OTG_FS_DCTL &= ~OTG_DCTL_RWUSIG;
 }