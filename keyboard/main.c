#include "display.h"
#include "hid.h"
#include "lib/key.h"
#include "lib/led.h"
#include "systick.h"
#include "usb.h"
// #include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
// #include <libopencm3/cm3/systick.h>

// #include <libopencm3/usb/usbd.h>

int main(void) {
	usbd_device *usbd_dev;

	// TODO 96MHz
	// PLLM 25
	// PLLN 192
	// PLLP 2
	// PLLQ 4
	rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

	key_setup();
	led_setup();
	systick_setup();
	display_setup();

	usbd_dev = usbd_setup();

	while (1) {
		usbd_poll(usbd_dev);
		hid_poll(usbd_dev);
		display_update();
	}
}
