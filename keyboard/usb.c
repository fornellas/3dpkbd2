#include "usb.h"
#include "hid.h"
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/dwc/otg_fs.h>

uint8_t usb_remote_wakeup_enabled = 0;
extern volatile uint32_t uptime_ms;

static char usb_serial_number[25];

uint8_t usb_suspended=0;

static const char *usb_strings[] = {
	"Fabio Pugliese Ornellas",
	"3D Printed Keyboard 2",
	usb_serial_number,
};

// Must be big enough to support HID in reports
uint8_t usbd_control_buffer[128];

const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	// For the lack of a better option, use Atmel's IDs.
	.idVendor = 0x03EB, // Atmel Corp.
	.idProduct = 0x2042, // LUFA Keyboard Demo Application
	.bcdDevice = 0x0100,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

const struct usb_interface interfaces[] = {
	{
		.num_altsetting = 1,
		.altsetting = &hid_iface,
	}
};

#define CONFIGURATION_VALUE 1

const struct usb_config_descriptor conf_descr = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = CONFIGURATION_VALUE,
	.iConfiguration = 0,
	.bmAttributes = (
		USB_CONFIG_ATTR_DEFAULT |
		USB_CONFIG_ATTR_SELF_POWERED |
		USB_CONFIG_ATTR_REMOTE_WAKEUP
	),
	.bMaxPower = 250, // 500 mAh

	.interface = interfaces,
};

static enum usbd_request_return_codes device_get_status(
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

static enum usbd_request_return_codes device_feature(
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

void set_config_callback(usbd_device *dev, uint16_t wValue);

void set_config_callback(usbd_device *dev, uint16_t wValue) {
	if(wValue != CONFIGURATION_VALUE)
		return;

	usbd_register_control_callback(
		dev,
		USB_REQ_TYPE_IN | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		device_get_status
	);

	usb_remote_wakeup_enabled = 0;
	usbd_register_control_callback(
		dev,
		USB_REQ_TYPE_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
		USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		device_feature
	);

	hid_set_config_callback(dev);

	usb_suspended = 0;
}

void reset_callback(void);

void reset_callback() {
	hid_poll_disable();
	usb_suspended = 0;
}

void resume_callback(void);

void resume_callback() {
	usb_suspended = 0;
}

void suspend_callback(void);

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