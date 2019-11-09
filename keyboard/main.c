#include "usb.h"
#include "../common/key.h"
#include "../common/led.h"
#include "../common/pin_reset.h"
#include <libopencm3/usb/usbd.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>

void soft_reset_if_pin_reset(void);

void soft_reset_if_pin_reset() {
	int pin_reset = PIN_RESET;
	RCC_CSR |= RCC_CSR_RMVF;

	if(pin_reset)
		scb_reset_system();
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

	usbd_dev = usbd_setup();

	while (1) {
		usbd_poll(usbd_dev);
	}
}
