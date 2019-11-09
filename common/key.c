#include "key.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#define KEY_RCC RCC_GPIOA
#define KEY_PORT GPIOA
#define KEY_GPIO GPIO0

void key_setup(void) {
	rcc_periph_clock_enable(KEY_RCC);
	gpio_mode_setup(KEY_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY_GPIO);
};

uint8_t key_pressed(void) {
	return (uint8_t)gpio_get(KEY_PORT, KEY_GPIO);
};