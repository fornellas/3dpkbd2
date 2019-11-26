#include <libopencm3/usb/usbd.h>

extern uint8_t usbd_control_buffer[65535];

#define USBD_STATE_RESET 0
#define USBD_STATE_SUSPENDED 1
#define USBD_STATE_ADDRESSED 2
#define USBD_STATE_CONFIGURED 3

extern uint8_t usbd_state;

usbd_device *usbd_setup(void);