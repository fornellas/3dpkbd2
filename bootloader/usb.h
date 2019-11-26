#include "lib/usb.h"

extern uint8_t usbd_control_buffer[65535];

usbd_device *usbd_setup(void);