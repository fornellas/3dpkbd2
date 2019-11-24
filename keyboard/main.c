#include "display.h"
#include "hid.h"
#include "lib/key.h"
#include "lib/led.h"
#include "usb.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/usbd.h>

//
// Variables
//

uint32_t volatile uptime_ms;

//
// Prototypes
// 

void systick_setup(void);

//
// Functions
//

void systick_setup(void) {
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_set_reload(rcc_ahb_frequency / 1000 - 1);
	systick_interrupt_enable();
	systick_clear();
	uptime_ms=0;
	systick_counter_enable();
}

void sys_tick_handler(void) {
	uptime_ms += 1;
}

int main(void) {
	usbd_device *usbd_dev;

	// TODO 96MHz
	// PLLM 25
	// PLLN 192
	// PLLP 2
	// PLLQ 4
	rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

	systick_setup();
	key_setup();
	led_setup();
	display_setup();

	usbd_dev = usbd_setup();

	while (1) {
		usbd_poll(usbd_dev);
		hid_poll(usbd_dev);
		display_update();
	}
}
