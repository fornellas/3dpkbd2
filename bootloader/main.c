#include "display.h"
#include "led.h"
#include "lib/addresses.h"
#include "lib/systick.h"
#include "usb.h"
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#define PIN_RESET_ONLY ( \
	( \
		RCC_CSR & RCC_CSR_PINRSTF \
	) && !( \
		RCC_CSR & ( \
			RCC_CSR_LPWRRSTF | \
			RCC_CSR_WWDGRSTF | \
			RCC_CSR_IWDGRSTF | \
			RCC_CSR_SFTRSTF | \
			RCC_CSR_PORRSTF | \
			RCC_CSR_BORRSTF \
		) \
	) \
)

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
	if(PIN_RESET_ONLY) {
		usbd_device *usbd_dev;

		RCC_CSR |= RCC_CSR_RMVF;

		rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);

		led_setup();
		systick_setup();
		display_setup();

		led_on();

		usbd_dev = usbd_setup();

		while (1) {
			display_update();
			usbd_poll(usbd_dev);
		}
	} else {
		RCC_CSR |= RCC_CSR_RMVF;

		jump_to_addr(MAIN_PROGRAM_BASE);
	}
}
