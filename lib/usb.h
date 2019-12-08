#include <libopencm3/usb/usbd.h>

#define USBD_STATE_RESET 0
#define USBD_STATE_ADDRESSED 1
#define USBD_STATE_CONFIGURED 2

extern uint8_t usbd_state;
extern uint8_t usbd_suspended;

usbd_device *usbd_setup_base(
	const struct usb_device_descriptor *,
	const struct usb_config_descriptor *,
	const char *const *,
	int,
	uint8_t *,
	uint16_t
);

void set_config_callback_base(usbd_device *, uint16_t);

#ifdef USBD_REMOTE_WAKEUP

extern uint8_t usbd_remote_wakeup_enabled;

void usdb_remote_wakeup_signal(void);

#endif