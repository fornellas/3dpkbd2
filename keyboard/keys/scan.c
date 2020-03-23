#include "../keys.h"
#include "scan.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <systick.h>
#include <libopencm3/stm32/i2c.h>
#include "lib/led.h"
#include "lib/systick.h"

#define RCC_GPIO_I2C RCC_GPIOB
#define RCC_I2C RCC_I2C1
#define GPIO_I2C GPIOB

#define I2C I2C1
#define GPIO_SCL GPIO8
#define GPIO_SDA GPIO9

#define MCP23017_ADDRESS 0x4E
#define MCP23017_BANK0_IOCON 0x0A
#define MCP23017_BANK1_IODIRA 0x00
#define MCP23017_BANK1_GPPUA 0x06
#define MCP23017_BANK1_GPIOA 0x09
#define MCP23017_BANK1_OLATA 0x0A
#define MCP23017_BANK1_IODIRB 0x10
#define MCP23017_BANK1_GPPUB 0x16
#define MCP23017_BANK1_GPIOB 0x19
#define MCP23017_BANK1_OLATB 0x1A

static uint8_t previous_key_state[ROWS][COLUMNS] = {};

static void i2c_setup(void) {
	rcc_periph_clock_enable(RCC_GPIO_I2C);
	rcc_periph_clock_enable(RCC_I2C);

	gpio_mode_setup(GPIO_I2C, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_SCL | GPIO_SDA);
	gpio_set_output_options(GPIO_I2C, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_SCL | GPIO_SDA);
	gpio_set_af(GPIO_I2C, GPIO_AF4, GPIO_SCL | GPIO_SDA);

	i2c_peripheral_disable(I2C);

	i2c_reset(I2C);

	i2c_set_fast_mode(I2C3);
	i2c_set_clock_frequency(I2C3, I2C_CR2_FREQ_42MHZ);
	i2c_set_ccr(I2C3, 35);
	i2c_set_trise(I2C3, 43);
	//i2c_set_speed(I2C3, 0);	

	i2c_peripheral_enable(I2C);
}

static void mcp23017_write(uint8_t reg, uint8_t value) {
	uint8_t data[2];

	data[0] = reg;
	data[1] = value;
	i2c_transfer7(I2C, MCP23017_ADDRESS, data, 2, NULL, 0);
}

static void mcp23017_read(uint8_t reg, uint8_t *value) {
	i2c_transfer7(I2C, MCP23017_ADDRESS, &reg, 1, NULL, 0);
	i2c_transfer7(I2C, MCP23017_ADDRESS, NULL, 0, value, 1);
}

static void mcp23017_setup(void) {
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
	mcp23017_write(MCP23017_BANK0_IOCON, 0b10100000);
	// Rows as input, columns as output
	mcp23017_write(MCP23017_BANK1_IODIRB, 0b01111111);
	mcp23017_write(MCP23017_BANK1_IODIRA, 0b00000000);
	// Row input pull up
	mcp23017_write(MCP23017_BANK1_GPPUB, 0b01111111);
	// Columns output high
	mcp23017_write(MCP23017_BANK1_OLATB, 0b10000000);
	mcp23017_write(MCP23017_BANK1_OLATA, 0b11111111);
}

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
	led_off();
	delay_ms(1);
	led_on();
	delay_ms(1);
	led_off();

	i2c_setup();

	led_on();
	delay_ms(1);
	led_off();

	uint8_t data[2];

	data[0] = 85;
	data[1] = 85;
	i2c_transfer7(I2C, 85, data, 2, NULL, 0);

	// mcp23017_setup();

	led_on();
	delay_ms(1);
	led_off();
	// // set_rows_as_intput_with_pullup();
	// // set_columns_as_output_and_low();
	// // keys_scan_reset();
	// led_off();
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
	return;
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