#include "hid.h"
#include "usb.h"
#include "../common/key.h"
#include "../common/led.h"
#include "../common/pin_reset.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
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

void soft_reset_if_pin_reset(void);

void systick_setup(void);

//
// Functions
//

void soft_reset_if_pin_reset() {
	int pin_reset = PIN_RESET;
	RCC_CSR |= RCC_CSR_RMVF;

	if(pin_reset)
		scb_reset_system();
}

void systick_setup(void) {
	uptime_ms=0;
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
	systick_set_reload((rcc_ahb_frequency / 8) / 1000);
	systick_counter_enable();
	systick_interrupt_enable();
}

void sys_tick_handler(void) {
	uptime_ms += 1;
}

int main(void) {
	usbd_device *usbd_dev;

	// https://github.com/libopencm3/libopencm3/issues/1119#issuecomment-549041942
	// DFU bootloader leaves state behind that prevents USB from working. If we
	// detect we came from the bootloader, we do a software reset to restore
	// vanilla USB state and allow it to work.
	soft_reset_if_pin_reset();

	// TODO 96MHz
	// PLLM 25
	// PLLN 192
	// PLLP 2
	// PLLQ 4
	rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

	key_setup();
	led_setup();
	systick_setup();

	usbd_dev = usbd_setup();

	while (1) {
		usbd_poll(usbd_dev);
		hid_poll(usbd_dev);
	}
}
