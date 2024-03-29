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
#include "lib/systick.h"

// Cherry MX datasheet states 5ms, though experimentation has shown that 5ms
// can yield to multiple key presses. As it is close to impossible to press a key
// more than 20 times a second, setting this to a conservative 50ms is enough
// to avoid ghost key presses without impacting intentional key presses
// (no matter how fast they are).
#define DEBOUNCE_MS 50

static uint8_t previous_key_state[ROWS][COLUMNS] = {};
static uint32_t edge_start_ms[ROWS][COLUMNS] = {};
uint8_t keys_scan_right_side_disconnected;
uint32_t last_key_trigger_ms = 0;

//
// Helpers
//

void debounce(
	bool read_state,
	uint8_t row,
	uint8_t column,
	bool *state,
	bool *pressed,
	bool *released,
	uint32_t now
);

void debounce(
	bool read_state,
	uint8_t row,
	uint8_t column,
	bool *state,
	bool *pressed,
	bool *released,
	uint32_t now
) {
	// Before
	if(read_state == previous_key_state[row][column]) {
		*state = previous_key_state[row][column];
		*pressed = false;
		*released = false;
	} else {
		// Leading Edge
		if(edge_start_ms[row][column] == 0) {
			if(previous_key_state[row][column]) {
				*pressed = false;
				*released = true;
			} else {
				*pressed = true;
				*released = false;
			}
			*state = read_state;
			previous_key_state[row][column] = read_state;
			edge_start_ms[row][column] = now;
		} else {
			// Debounce
			if(now - edge_start_ms[row][column] < DEBOUNCE_MS) {
				*state = previous_key_state[row][column];
				*pressed = false;
				*released = false;
			// Tailing Edge
			} else {
				if(previous_key_state[row][column]) {
					*pressed = false;
					*released = true;
				} else {
					*pressed = true;
					*released = false;
				}
				*state = read_state;
				previous_key_state[row][column] = read_state;
				edge_start_ms[row][column] = 0;
			}
		}
	}
}

//
// setup
//

void keys_scan_state_reset() {
	for(uint8_t row=0 ; row < ROWS ; row++)
		for(uint8_t column=0; column < COLUMNS ; column++) {
			previous_key_state[row][column] = 0;
			edge_start_ms[row][column] = 0;
		}
}

static void setup_left_side(void) {
	//
	// Set rows as output and high
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
	// Set columns as input with pull-down
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
	if(keys_scan_right_side_disconnected)
		i2c_setup();

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
	if(mcp23017_write(MCP23017_BANK1_IODIRA, 0b00000000)){
		keys_scan_right_side_disconnected = 1;
		return;
	}
	if(mcp23017_write(MCP23017_BANK1_IODIRB, 0b01111111)){
		keys_scan_right_side_disconnected = 1;
		return;
	}


	// Row input pull up
	if(mcp23017_write(MCP23017_BANK1_GPPUB, 0b01111111)){
		keys_scan_right_side_disconnected = 1;
		return;
	}

	// Columns output high
	if(mcp23017_write(MCP23017_BANK1_OLATA, 0b11111111)){
		keys_scan_right_side_disconnected = 1;
		return;
	}
	if(mcp23017_write(MCP23017_BANK1_OLATB, 0b10000000)){
		keys_scan_right_side_disconnected = 1;
		return;
	}

	keys_scan_right_side_disconnected = 0;
}

void keys_scan_setup(void) {
	keys_scan_state_reset();
	setup_left_side();
	keys_scan_right_side_disconnected = 1;
	setup_right_side();
}

//
// left
//

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

static void keys_scan_left(void (*callback)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, void *), void *data) {
	uint32_t now;

	now = uptime_ms();

	for (uint8_t row = 0; row < ROWS ; row++) {
		uint8_t any_state_active;
		bool state;
		bool pressed;
		bool released;

		any_state_active = 0;
		set_left_row_level(row, 1);
		for (uint8_t column = 0 ; column < LEFT_COLUMNS ; column++) {
			uint16_t read_state;

			read_state = get_left_column(column);

			debounce(read_state, row, column, &state, &pressed, &released, now);

			if(state || pressed || released) {
				last_key_trigger_ms = now;
				(*callback)(row, column, state, pressed, released, data);
			}
		}
		set_left_row_level(row, 0);
		// Due to line capacitances we have to wait for the previous row high
		// signal to be drained by the pull down resistor so the column input
		// signal is back to logic low.
		if(any_state_active) {
			// ~10us
			for(uint16_t i=0 ; i < 200 ; i++)
				__asm__("nop");
		}
	}
}

//
// right
//

static uint8_t clear_right_column(uint8_t column) {
    if(column == 7) {
		if(mcp23017_write(MCP23017_BANK1_OLATB, 0x00)){
			keys_scan_right_side_disconnected = 1;
			return 1;
		}
    } else {
        uint8_t bit;

        bit = 15 - column;

        if(mcp23017_write(MCP23017_BANK1_OLATA, 0xFF & ~(1 << bit))) {
            keys_scan_right_side_disconnected = 1;
            return 1;
        }
    }

    return 0;
}

static uint8_t set_right_column(uint8_t column) {
    if(column == 7) {
        if(mcp23017_write(MCP23017_BANK1_OLATB, 0b10000000)){
            keys_scan_right_side_disconnected = 1;
            return 1;
        }
    } else {
        if(mcp23017_write(MCP23017_BANK1_OLATA, 0xFF)) {
            keys_scan_right_side_disconnected = 1;
            return 1;
        }
    }

    return 0;
}

static uint8_t get_right_rows(uint8_t *rows_state) {
    if(mcp23017_read(MCP23017_BANK1_GPIOB, rows_state)){
        keys_scan_right_side_disconnected = 1;
        return 1;
    }

    return 0;
}

static void keys_scan_right(void (*callback)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, void *), void *data) {
	uint32_t now;

	now = uptime_ms();

	if(keys_scan_right_side_disconnected) {
		setup_right_side();
		if(keys_scan_right_side_disconnected)
			return;
	}

	for (uint8_t column = LEFT_COLUMNS; column < COLUMNS ; column++) {
		bool state;
		bool pressed;
		bool released;
        uint8_t rows_state;


		if(clear_right_column(column))
            return;

        if(get_right_rows(&rows_state))
            return;

		for (uint8_t row = 0 ; row < ROWS ; row++) {
			uint8_t read_state;

			read_state = !(rows_state & (1 << row));

			debounce(read_state, row, column, &state, &pressed, &released, now);

			if(state || pressed || released) {
				last_key_trigger_ms = now;
				(*callback)(row, column, state, pressed, released, data);
			}
		}
		if(set_right_column(column))
            return;
	}
}

//
// keys_scan
//

void keys_scan(void (*callback)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, void *), void *data) {
	keys_scan_left(callback, data);
	keys_scan_right(callback, data);
}
