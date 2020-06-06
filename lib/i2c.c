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

void i2c_setup(void) {
	rcc_periph_clock_enable(RCC_GPIO_I2C);
	rcc_periph_clock_enable(RCC_I2C);

	gpio_mode_setup(GPIO_I2C, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_SCL | GPIO_SDA);
	gpio_set_output_options(GPIO_I2C, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_SCL | GPIO_SDA);
	gpio_set_af(GPIO_I2C, GPIO_AF4, GPIO_SCL | GPIO_SDA);

	i2c_peripheral_disable(I2C);

	i2c_reset(I2C);

	i2c_set_fast_mode(I2C);
	i2c_set_clock_frequency(I2C, I2C_CR2_FREQ_42MHZ);
	i2c_set_ccr(I2C, 35);
	i2c_set_trise(I2C, 43);
	//i2c_set_speed(I2C, 0);

	i2c_peripheral_enable(I2C);

	i2c_set_own_7bit_slave_address(I2C, 0x00);
}

static uint8_t abort_if_error_condition(uint32_t start_ms) {
	if(
		(uptime_ms() - start_ms >= TIMEOUT_MS)
		|| (
			I2C_SR1(I2C) & (
				I2C_SR1_BERR |
				I2C_SR1_ARLO |
				I2C_SR1_AF |
				I2C_SR1_TIMEOUT
			)
		)
	) {
		i2c_setup();
		return 1;
	}

	return 0;
}

uint8_t i2c_write(uint8_t addr, uint8_t *data, size_t len) {
	uint32_t start_ms;

	// Ensure lines are not busy
	if((I2C_SR2(I2C) & I2C_SR2_BUSY)){
		i2c_setup();
		return 1;
	}

	// Send start condition
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

	// Send slave address
	start_ms = uptime_ms();
	i2c_send_7bit_address(I2C, addr, I2C_WRITE);
	while (!(I2C_SR1(I2C) & I2C_SR1_ADDR))
		if(abort_if_error_condition(start_ms))
			return 1;;
	(void)I2C_SR2(I2C);

	// Send data
	for (size_t i = 0; i < len; i++) {
		start_ms = uptime_ms();
		i2c_send_data(I2C, data[i]);
		while (!(I2C_SR1(I2C) & (I2C_SR1_BTF)))
			if(abort_if_error_condition(start_ms))
				return 1;
	}

	// Send stop
	i2c_send_stop(I2C);
	return 0;
}

uint8_t i2c_read(uint8_t addr, uint8_t *data, size_t len) {
	uint32_t start_ms;

	i2c_send_start(I2C);

	i2c_enable_ack(I2C);

	/* Wait for master mode selected */
	while (!((I2C_SR1(I2C) & I2C_SR1_SB) & (I2C_SR2(I2C) & (I2C_SR2_MSL | I2C_SR2_BUSY))));

	start_ms = uptime_ms();
	i2c_send_7bit_address(I2C, addr, I2C_READ);
	while (!(I2C_SR1(I2C) & I2C_SR1_ADDR)) {
		if(uptime_ms() - start_ms >= TIMEOUT_MS){
			i2c_send_stop(I2C);
			return 1;
		}
	};

	/* Clearing ADDR condition sequence. */
	(void)I2C_SR2(I2C);

	for (size_t i = 0; i < len; ++i) {
		if (i == len - 1) {
			i2c_disable_ack(I2C);
		}
		start_ms = uptime_ms();
		while (!(I2C_SR1(I2C) & I2C_SR1_RxNE)) {
			if(uptime_ms() - start_ms >= TIMEOUT_MS){
				i2c_send_stop(I2C);
				return 1;
			}
		}
		data[i] = i2c_get_data(I2C);
	}
	i2c_send_stop(I2C);

	return 0;
}
