#include <libopencm3/usb/usbstd.h>
#include <libopencm3/usb/usbd.h>

extern const struct usb_interface_descriptor hid_iface;

void keyboard_set_config(usbd_device *dev, uint16_t wValue);