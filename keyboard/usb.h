#include "lib/usb.h"
#include "descriptors.h"

// Must be as big as the biggest HID report
extern uint8_t usbd_control_buffer[sizeof(struct hid_in_report_data)];

extern uint8_t usb_remote_wakeup_enabled;

usbd_device *usbd_setup(void);