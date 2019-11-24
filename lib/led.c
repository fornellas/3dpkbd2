#include "led.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#define RCC_LED RCC_GPIOC
#define LED_PORT GPIOC
#define LED_GPIO GPIO13

void led_setup(void) {
	rcc_periph_clock_enable(RCC_LED);
	gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GPIO);
	led_off();
}

void led_on(void) {
	gpio_clear(LED_PORT, LED_GPIO);
}

void led_off(void) {
	gpio_set(LED_PORT, LED_GPIO);
}

void led_toggle(void) {
	gpio_toggle(LED_PORT, LED_GPIO);
}