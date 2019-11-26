#include <libopencm3/usb/usbstd.h>
#include <libopencm3/usb/dfu.h>

extern char usb_serial_number[25];

#define USB_STRINGS_NUM 4
extern const char *usb_strings[USB_STRINGS_NUM];

#define CONFIGURATION_VALUE 1

extern const struct usb_device_descriptor dev_descr;

extern const struct usb_config_descriptor conf_descr;

extern const struct usb_interface interfaces[];

#define DFU_INTERFACE_NUMBER 0

extern const struct usb_interface_descriptor dfu_interface;

extern const struct usb_dfu_descriptor dfu_function;
