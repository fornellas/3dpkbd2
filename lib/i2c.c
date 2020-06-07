#include "i2c.h"
#include "systick.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>

#define RCC_GPIO_I2C RCC_GPIOB
#define RCC_I2C RCC_I2C1
#define GPIO_I2C GPIOB

#define GPIO_SCL GPIO8
#define GPIO_SDA GPIO9

#define TIMEOUT_MS 2

static void setup_io(void) {
	rcc_periph_clock_enable(RCC_GPIO_I2C);

	gpio_mode_setup(GPIO_I2C, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_SCL | GPIO_SDA);
	gpio_set_output_options(GPIO_I2C, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_SCL | GPIO_SDA);
	gpio_set_af(GPIO_I2C, GPIO_AF4, GPIO_SCL | GPIO_SDA);
}

static void setup_peripheral(void) {
	rcc_periph_clock_enable(RCC_I2C);

	i2c_peripheral_disable(I2C);

	i2c_reset(I2C);

	i2c_set_speed(I2C, i2c_speed_fm_400k, rcc_apb1_frequency / 1e6);

	// Without aggressive filter we see frequent communication loss
	I2C_FLTR(I2C) = (I2C_FLTR(I2C) & ~I2C_FLTR_DNF_MASK) | (15<<I2C_FLTR_DNF_SHIFT);

	i2c_peripheral_enable(I2C);

	i2c_set_own_7bit_slave_address(I2C, 0x00);
}

void i2c_setup(void) {
	setup_io();
	setup_peripheral();
}

static void reset_and_setup_if_busy(void) {
	// Deal with stuck BUSY
	if ((I2C_SR2(I2C) & I2C_SR2_BUSY))
		setup_peripheral();
}

static uint8_t abort_if_error_condition(uint32_t start_ms) {
	if(
		(uptime_ms() - start_ms >= TIMEOUT_MS)
		|| (
			I2C_SR1(I2C) & (
				I2C_SR1_BERR
				| I2C_SR1_ARLO
				| I2C_SR1_AF
				| I2C_SR1_TIMEOUT
			)
		)
	) {
		setup_peripheral();
		return 1;
	}

	return 0;
}

static uint8_t send_start_condition(void) {
	uint32_t start_ms;

	start_ms = uptime_ms();
	i2c_send_start(I2C);
	while (!(I2C_SR2(I2C) & I2C_SR2_BUSY))
		if(abort_if_error_condition(start_ms))
			return 1;
	while(!(I2C_SR2(I2C) & I2C_SR2_MSL))
		if(abort_if_error_condition(start_ms))
			return 1;
	while(!(I2C_SR1(I2C) & I2C_SR1_SB))
		if(abort_if_error_condition(start_ms))
			return 1;

	return 0;
}

static uint8_t send_slave_address(uint8_t addr) {
	uint32_t start_ms;

	start_ms = uptime_ms();
	i2c_send_7bit_address(I2C, addr, I2C_WRITE);
	while (!(I2C_SR1(I2C) & I2C_SR1_ADDR))
		if(abort_if_error_condition(start_ms))
			return 1;;
	(void)I2C_SR2(I2C);

	return 0;
}

uint8_t i2c_write(uint8_t addr, uint8_t *data, size_t len) {
	reset_and_setup_if_busy();

	if(send_start_condition())
		return 1;

	if(send_slave_address(addr))
		return 1;

	for (size_t i = 0; i < len; i++) {
		uint32_t start_ms;

		start_ms = uptime_ms();
		i2c_send_data(I2C, data[i]);
		while (!(I2C_SR1(I2C) & (I2C_SR1_BTF)))
			if(abort_if_error_condition(start_ms))
				return 1;
	}

	i2c_send_stop(I2C);

	return 0;
}

uint8_t i2c_read(uint8_t addr, uint8_t *data, size_t len) {
	reset_and_setup_if_busy();

	if(send_start_condition())
		return 1;

	if(send_slave_address(addr))
		return 1;

	for (size_t i = 0; i < len; ++i) {
		uint32_t start_ms;

		if (i == len - 1)
			i2c_disable_ack(I2C);

		start_ms = uptime_ms();
		while (!(I2C_SR1(I2C) & I2C_SR1_RxNE))
			if(abort_if_error_condition(start_ms))
				return 1;
		data[i] = i2c_get_data(I2C);
	}

	i2c_send_stop(I2C);

	return 0;
}
