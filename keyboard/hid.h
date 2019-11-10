#include <libopencm3/usb/usbstd.h>
#include <libopencm3/usb/usbd.h>

extern const struct usb_interface_descriptor hid_iface;

void hid_poll(usbd_device *dev);
void hid_set_config_callback(usbd_device *dev);