#include <libopencm3/usb/usbd.h>

#define USBD_STATE_RESET 0
#define USBD_STATE_SUSPENDED 1
#define USBD_STATE_ADDRESSED 2
#define USBD_STATE_CONFIGURED 3

extern uint8_t usbd_state;

usbd_device *usbd_setup_base(
	const struct usb_device_descriptor *,
	const struct usb_config_descriptor *,
	const char *const *,
	int,
	uint8_t *,
	uint16_t
);

void set_config_callback_base(usbd_device *, uint16_t);

void usdb_remote_wakeup_signal(void);