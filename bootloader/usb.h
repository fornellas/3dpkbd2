#include <libopencm3/usb/usbd.h>

extern uint8_t usbd_control_buffer[1024];

usbd_device *usbd_setup(void);