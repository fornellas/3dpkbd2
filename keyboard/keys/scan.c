#include "../keys.h"
#include "scan.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <systick.h>

static uint8_t previous_key_state[ROWS][COLUMNS] = {};

static void set_rows_as_intput_with_pullup(void) {
	// Row 0 > B12
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
	gpio_set(GPIOB, GPIO12);
	// Row 1 > B13
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
	gpio_set(GPIOB, GPIO13);
	// Row 2 > B14
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO14);
	gpio_set(GPIOB, GPIO14);
	// Row 3 > B15
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO15);
	gpio_set(GPIOB, GPIO15);
	// Row 4 > A8
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8);
	gpio_set(GPIOA, GPIO8);
	// Row 5 > A9
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO9);
	gpio_set(GPIOA, GPIO9);
	// Row 6 > A10
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO10);
	gpio_set(GPIOA, GPIO10);
}

static void set_columns_as_output_and_low(void) {
	// Column 0       A1
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO1);
	// Column 1       A0
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO0);
	// Column 2       B3
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO3);
	// Column 3       B4
	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO4);
	// Column 4       B5
	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO5);
	// Column 5       B6
	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO6);
	// Column 6       B7
	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO7);
}

void keys_scan_reset() {
	for(uint8_t row=0 ; row < ROWS ; row++)
		for(uint8_t column=0; column < COLUMNS ; column++)
			previous_key_state[row][column] = 0;
}

void keys_scan_setup(void) {
	set_rows_as_intput_with_pullup();
	set_columns_as_output_and_low();
	keys_scan_reset();
}

static void set_row_level(uint8_t row, uint8_t level) {
	switch(row) {
	case 0:
		if (level)
			gpio_set(GPIOB, GPIO12);
		else
			gpio_clear(GPIOB, GPIO12);
		break;
	case 1:
		if (level)
			gpio_set(GPIOB, GPIO13);
		else
			gpio_clear(GPIOB, GPIO13);
		break;
	case 2:
		if (level)
			gpio_set(GPIOB, GPIO14);
		else
			gpio_clear(GPIOB, GPIO14);
		break;
	case 3:
		if (level)
			gpio_set(GPIOB, GPIO15);
		else
			gpio_clear(GPIOB, GPIO15);
		break;
	case 4:
		if (level)
			gpio_set(GPIOA, GPIO8);
		else
			gpio_clear(GPIOA, GPIO8);
		break;
	case 5:
		if (level)
			gpio_set(GPIOA, GPIO9);
		else
			gpio_clear(GPIOA, GPIO9);
		break;
	case 6:
		if (level)
			gpio_set(GPIOA, GPIO10);
		else
			gpio_clear(GPIOA, GPIO10);
		break;
	}
	// TODO right side
}

static uint16_t get_column(uint8_t column) {
	switch (column) {
	case 0:
		return gpio_get(GPIOA, GPIO1);
	case 1:
		return gpio_get(GPIOA, GPIO0);
	case 2:
		return gpio_get(GPIOB, GPIO3);
	case 3:
		return gpio_get(GPIOB, GPIO4);
	case 4:
		return gpio_get(GPIOB, GPIO5);
	case 5:
		return gpio_get(GPIOB, GPIO6);
	case 6:
		return gpio_get(GPIOB, GPIO7);
	}
	// TODO right side
	return 0;
}

void keys_scan(void (*callback)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, void *), void *data) {
	for (uint8_t row = 0; row < ROWS ; row++) {
		uint8_t any_pressed;
		uint8_t state;
		uint8_t pressed;
		uint8_t released;

		any_pressed = 0;
		set_row_level(row, 1);
		for (uint8_t column = 0 ; column < COLUMNS ; column++) {
			if (get_column(column)){
				any_pressed = 1;
				state = 1;
				pressed = !previous_key_state[row][column];
				released = 0;
				previous_key_state[row][column] = 1;
			}else{
				state = 0;
				pressed = 0;
				released = previous_key_state[row][column];
				previous_key_state[row][column] = 0;
			}
			if(state || pressed || released)
				(*callback)(row, column, state, pressed, released, data);
		}
		set_row_level(row, 0);
		// Due to line capacitances we have to wait for the previous row high
		// signal to be drained by the pull down resistor so the column input
		// signal is back to logic low.
		if(any_pressed){
			// ~10us
			for(uint16_t i=0 ; i < 200 ; i++)
				__asm__("nop");
		}
	}
}