#include <libopencm3/usb/usbstd.h>
#include <libopencm3/usb/usbd.h>

extern uint8_t hid_protocol;

void hid_poll(usbd_device *dev);
void hid_set_config_callback(usbd_device *dev);