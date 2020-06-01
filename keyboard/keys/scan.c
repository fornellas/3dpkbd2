#include "../keys.h"
#include "scan.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <systick.h>
#include <libopencm3/stm32/i2c.h>
#include "lib/led.h"
#include "lib/systick.h"
#include "lib/i2c.h"
#include "lib/mcp23017.h"

//
// Common
//

static uint8_t previous_key_state[ROWS][COLUMNS] = {};
uint8_t keys_scan_right_side_disconnected;

void keys_scan_reset() {
	for(uint8_t row=0 ; row < ROWS ; row++)
		for(uint8_t column=0; column < COLUMNS ; column++)
			previous_key_state[row][column] = 0;
}

static void setup_left_side(void) {
	//
	// Set rows as input with pull-up
	//

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

	//
	// Set columns as output and low
	//

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

static void setup_right_side(void) {
	// Rows Input, Pull up
	// 0 PB0
	// 1 PB1
	// 2 PB2
	// 3 PB3
	// 4 PB4
	// 5 PB5
	// 6 PB6
	// Columns Output, High
	// 7 PB7
	// 8 PA7
	// 9 PA6
	// 10 PA5
	// 11 PA4
	// 12 PA3
	// 13 PA2
	// 14 PA1
	// 15 PA0

	// BANK=1, SEQOP=1
	if(mcp23017_write(MCP23017_BANK0_IOCON, 0b10100000)){
		keys_scan_right_side_disconnected = 1;
		return;
	}
	// Rows as input, columns as output
	if(mcp23017_write(MCP23017_BANK1_IODIRB, 0b01111111)){
		keys_scan_right_side_disconnected = 1;
		return;
	}
	if(mcp23017_write(MCP23017_BANK1_IODIRA, 0b00000000)){
		keys_scan_right_side_disconnected = 1;
		return;
	}
	// Row input pull up
	if(mcp23017_write(MCP23017_BANK1_GPPUB, 0b01111111)){
		keys_scan_right_side_disconnected = 1;
		return;
	}
	// Columns output high
	if(mcp23017_write(MCP23017_BANK1_OLATB, 0b10000000)){
		keys_scan_right_side_disconnected = 1;
		return;
	}
	if(mcp23017_write(MCP23017_BANK1_OLATA, 0b11111111)){
		keys_scan_right_side_disconnected = 1;
		return;
	}
	keys_scan_right_side_disconnected = 0;
}

void keys_scan_setup(void) {
	keys_scan_reset();
	setup_left_side();
	i2c_setup();
	keys_scan_right_side_disconnected = 1;
	setup_right_side();
}

static void set_left_row_level(uint8_t row, uint8_t level) {
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
}

static void set_right_row_level(uint8_t row, uint8_t level) {
	if(keys_scan_right_side_disconnected)
		return;
	// TODO
	(void)row;
	(void)level;
}

static void set_row_level(uint8_t row, uint8_t level) {
	set_left_row_level(row, level);
	set_right_row_level(row, level);
}

static uint16_t get_left_column(uint8_t column) {
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
	return 0;
}

static uint16_t get_right_column(uint8_t column) {
	if(keys_scan_right_side_disconnected)
		return 0;
	// TODO
	(void)column;
	return 0;
}

static uint16_t get_column(uint8_t column) {
	if(column < LEFT_COLUMNS)
		return get_left_column(column);
	else
		return get_right_column(column);
}

void keys_scan(void (*callback)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, void *), void *data) {
	if(keys_scan_right_side_disconnected)
		setup_right_side();

	for (uint8_t row = 0; row < ROWS ; row++) {
		uint8_t any_pressed;
		uint8_t any_triggered;
		uint8_t state;
		uint8_t pressed;
		uint8_t released;

		any_pressed = 0;
		any_triggered = 0;
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
			if(pressed || released)
				any_triggered = 1;
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
		// Debouncing
		if(any_triggered){
			// ~1ms
			for(uint16_t i=0 ; i < 20000 ; i++)
				__asm__("nop");
		}
	}
}