#include "systick.h"
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>

static uint32_t volatile _uptime_ms;

void sys_tick_handler(void);

void sys_tick_handler(void) {
	_uptime_ms += 1;
}

void systick_setup(void) {
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_set_reload(rcc_ahb_frequency / 1000 - 1);
	systick_interrupt_enable();
	systick_clear();
	_uptime_ms=0;
	systick_counter_enable();
}

void delay_ms(uint32_t ms) {
	uint32_t start_ms;

	start_ms = _uptime_ms;

	while(_uptime_ms - start_ms < ms);
}

uint32_t uptime_ms(void) {
	return _uptime_ms;
}