#include "usb.h"
#include "hid.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/desig.h>

static char usb_serial_number[25];

static const char *usb_strings[] = {
	"Fabio Pugliese Ornellas",
	"3D Printed Keyboard 2",
	usb_serial_number,
	"Boot Keyboard Configuraton"
	"Boot Keyboard Interface"
};

uint8_t usbd_control_buffer[128];

// https://www.beyondlogic.org/usbnutshell/usb5.shtml#DeviceDescriptors
const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	// STMicroelectronics
	.idVendor = 0x0483,
	// Joystick in FS Mode
	.idProduct = 0x5710,
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

// https://www.beyondlogic.org/usbnutshell/usb5.shtml#ConfigurationDescriptors
const struct usb_config_descriptor conf_descr = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	// FIXME Includes the combined length of all returned descriptors
	// (configuration, interface, endpoint, and HID) returned
	// for this configuration. This value includes the HID
	// descriptor but none of the other HID class descriptors
	// (report or designator). 
	.wTotalLength = 0,
	// TODO bind to interfaces
	.bNumInterfaces = 1,
	.bConfigurationValue = CONFIGURATION_VALUE,
	.iConfiguration = 4,
	.bmAttributes = (
		(1<<7) | // D7 Reserved, set to 1. (USB 1.0 Bus Powered)
		(1<<5) // D5 Remote Wakeup
	),
	.bMaxPower = 250, // 500 mAh

	.interface = interfaces,
};

void set_config_callback(usbd_device *dev, uint16_t wValue);

void set_config_callback(usbd_device *dev, uint16_t wValue) {
	if(wValue != CONFIGURATION_VALUE)
		return;

	hid_set_config_callback(dev);
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
		usb_strings, 5,
		usbd_control_buffer,
		sizeof(usbd_control_buffer)
	);

	usbd_register_set_config_callback(usbd_dev, set_config_callback);
	// TODO
	// usbd_register_reset_callback();
	// TODO
	// usbd_register_resume_callback();
	// TODO
	// usbd_register_set_altsetting_callback();
	// TODO
	// usbd_register_sof_callback();
	// TODO
	// usbd_register_suspend_callback();

	return usbd_dev;
}