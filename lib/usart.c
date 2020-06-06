#include "usart.h"
#include <stdio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#define USART_RCC_PORT RCC_GPIOA
#define USART_RCC RCC_USART1
#define USART_PORT GPIOA
#define USART_GPIO GPIO15
#define USART_GPIO_AF GPIO_AF7
#define USART USART1
#define USART_BAUDRATE 115200
#define USART_BITS 8
#define USART_STOPBITS USART_STOPBITS_1_5
#define USART_MODE USART_MODE_TX
#define USART_PARITY USART_PARITY_NONE
#define USART_FLOW_CONTROL USART_FLOWCONTROL_NONE

static ssize_t usart_write(void *cookie, const char *buf, size_t size)
{
	size_t i;

	(void)cookie;

	for (i = 0; i < size; i++) {
		char c = buf[i];
		usart_send_blocking(USART1, c);
	}
	return size;
}

void usart_setup() {
	rcc_periph_clock_enable(USART_RCC_PORT);
	rcc_periph_clock_enable(USART_RCC);
	gpio_mode_setup(USART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, USART_GPIO);
	gpio_set_af(USART_PORT, USART_GPIO_AF, USART_GPIO);

	usart_set_baudrate(USART, USART_BAUDRATE);
	usart_set_databits(USART, USART_BITS);
	usart_set_stopbits(USART, USART_STOPBITS);
	usart_set_mode(USART, USART_MODE);
	usart_set_parity(USART, USART_PARITY);
	usart_set_flow_control(USART, USART_FLOW_CONTROL);

	usart_enable(USART);

	cookie_io_functions_t usart_output_fns = {
		.read  = NULL,
		.write = usart_write,
		.seek  = NULL,
		.close = NULL
	};

	stdout = fopencookie(NULL, "w", usart_output_fns);
	setvbuf(stdout, NULL, _IONBF, 0);
	setlinebuf(stdout);
}