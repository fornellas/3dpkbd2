#include "descriptors.h"
#include "dfu.h"
#include "usb.h"
#include <libopencm3/stm32/desig.h>

//
// Variables
//

uint8_t usbd_control_buffer[];

//
// Prototypes
//

static void set_config_callback(usbd_device *, uint16_t);

//
// Functions
//

static void set_config_callback(usbd_device *dev, uint16_t wValue) {
	if(wValue != CONFIGURATION_VALUE)
		return;

	set_config_callback_base(dev, wValue);

	dfu_set_config_callback(dev);
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