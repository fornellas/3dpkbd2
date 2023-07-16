#include "display.h"
#include "hid.h"
#include "lib/led.h"
#include "systick.h"
#include "keys.h"
#include "usb.h"
#include <libopencm3/stm32/rcc.h>
#include "keys/layers.h"

int main(void) {
	usbd_device *usbd_dev;

	rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

	systick_setup();
	led_setup();
	keys_setup();
	display_setup();

	usbd_dev = usbd_setup();

	while (1) {
		display_update();
		usbd_poll(usbd_dev);
		hid_poll(usbd_dev);
		// Mac seems to always set idle rate, so we use it to detect whether we're connected to it
		if(hid_idle_rate_ms_boot) {
			layers_state[LAYER_MAC] = 1;
		} else {
			layers_state[LAYER_MAC] = 0;
		}
	}
}
