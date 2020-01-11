#include "descriptors.h"
#include "usb.h"

char usb_serial_number[25];

const char *usb_strings[USB_STRINGS_NUM] = {
	"Fabio Pugliese Ornellas",
	"DFU for 3D Printed Keyboard 2",
	usb_serial_number,
	"DFU 1.1",
};

const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = USB_VID,
	.idProduct = USB_PID,
	.bcdDevice = 0x0100,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	// One configuration only for DFU.
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
	// Alternate setting. Must be zero.
	.bAlternateSetting = 0,
	// Only the control pipe is used.
	.bNumEndpoints = 0,
	// DFU
	.bInterfaceClass = USB_CLASS_DFU,
	// Device Firmware Upgrade Code
	.bInterfaceSubClass = 1,
	// DFU mode protocol.
	.bInterfaceProtocol = 2,
	// Index of string descriptor for this interface
	.iInterface = 4,

	.extra = &dfu_function,
	.extralen = sizeof(dfu_function),
};

const struct usb_dfu_descriptor dfu_function = {
	.bLength = sizeof(struct usb_dfu_descriptor),
	.bDescriptorType = DFU_FUNCTIONAL,
	.bmAttributes = (
		// download capable (bitCanDnload)
		USB_DFU_CAN_DOWNLOAD |
		// upload capable (bitCanUpload)
		// USB_DFU_CAN_UPLOAD |
		// device is able to communicate via USB after Manifestation phase.
		// (bitManifestationTolerant)
		USB_DFU_MANIFEST_TOLERANT
		// Device will perform a bus detach-attach sequence when it receives
		// a DFU_DETACH request. The host must not issue a USB Reset.
		// (bitWillDetach)
		// USB_DFU_WILL_DETACH
	),
	// Time, in milliseconds, that the device will wait after receipt of the
	// DFU_DETACH request. If this time elapses without a USB reset, then the
	// device will terminate the Reconfiguration phase and revert back to
	// normal operation. This represents the maximum time that the device can
	// wait (depending on its timers, etc.). The host may specify a shorter
	// timeout in the DFU_DETACH request.
	.wDetachTimeout = 0,
	// Maximum number of bytes that the device can accept per control-write
	// transaction.
	.wTransferSize = sizeof(usbd_control_buffer),
	// Numeric expression identifying the version of the DFU Specification
	// release.
	.bcdDFUVersion = 0x0110,
};