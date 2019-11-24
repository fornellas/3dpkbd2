#include "descriptors.h"

char usb_serial_number[25];

const char *usb_strings[] = {
	"Fabio Pugliese Ornellas",
	"DFU for 3D Printed Keyboard 2",
	usb_serial_number,
	"@Internal Flash  /0x08000000/04*016Kg,01*064Kg,03*128Kg",
};

const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0xDF11,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

const struct usb_config_descriptor conf_descr = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = CONFIGURATION_VALUE,
	.iConfiguration = 0,
	.bmAttributes = (
		USB_CONFIG_ATTR_DEFAULT |
		USB_CONFIG_ATTR_SELF_POWERED
	),
	.bMaxPower = 250, // 500 mAh

	.interface = interfaces,
};

const struct usb_interface interfaces[] = {
	{
	.num_altsetting = 1,
	.altsetting = &dfu_interface,
	}
};

const struct usb_interface_descriptor dfu_interface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = DFU_INTERFACE_NUMBER,
	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = 0xFE, /* Device Firmware Upgrade */
	.bInterfaceSubClass = 1,
	.bInterfaceProtocol = 2,

	/* The ST Microelectronics DfuSe application needs this string.
	 * The format isn't documented... */
	.iInterface = 4,

	.extra = &dfu_function,
	.extralen = sizeof(dfu_function),
};

const struct usb_dfu_descriptor dfu_function = {
	.bLength = sizeof(struct usb_dfu_descriptor),
	.bDescriptorType = DFU_FUNCTIONAL,
	.bmAttributes = USB_DFU_CAN_DOWNLOAD | USB_DFU_WILL_DETACH,
	.wDetachTimeout = 255,
	.wTransferSize = sizeof(usbd_control_buffer),
	.bcdDFUVersion = 0x011A,
};