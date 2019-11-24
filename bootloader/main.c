#include "addresses.h"
#include "usb.h"
#include "../common/led.h"
#include "../common/pin_reset.h"
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

void jump_to_addr(uint32_t addr);

void jump_to_addr(uint32_t addr) {
	// Set vector table base address.
	SCB_VTOR = addr;
	// Initialize master stack pointer.
	asm volatile("msr msp, %0"::"g"(*(volatile uint32_t *)addr));
	// Jump to application.
	((void (*)(void)) *((uint32_t*) (addr + 4)))();
}

int main(void) {
	if(PIN_RESET) {
		usbd_device *usbd_dev;

		rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

		led_setup();
		led_on();

		usbd_dev = usbd_setup();

		while (1) {
			usbd_poll(usbd_dev);
		}
	} else {
		jump_to_addr(MAIN_PROGRAM_BASE);
	}
}
