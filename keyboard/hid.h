#include "descriptors.h"
#include <libopencm3/usb/usbstd.h>
#include <libopencm3/usb/usbd.h>

extern uint8_t hid_protocol;
extern int16_t idle_rate_ms;
extern hid_out_report_data led_report;

void hid_poll(usbd_device *dev);
void hid_set_config_callback(usbd_device *dev);