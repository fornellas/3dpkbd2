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

#define TIMEOUT_MS 1

static void setup_io(void) {
	rcc_periph_clock_enable(RCC_GPIO_I2C);
	rcc_periph_clock_enable(RCC_I2C);

	gpio_mode_setup(GPIO_I2C, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_SCL | GPIO_SDA);
	gpio_set_output_options(GPIO_I2C, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_SCL | GPIO_SDA);
	gpio_set_af(GPIO_I2C, GPIO_AF4, GPIO_SCL | GPIO_SDA);
}

static void setup_peripheral(void) {
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

void i2c_setup(void) {
	setup_io();
	setup_peripheral();
}

static uint8_t wait_not_busy(void) {
	uint32_t start_time;

	start_time = uptime_ms();
	while ((I2C_SR2(I2C) & I2C_SR2_BUSY)) {
		if(uptime_ms() - start_time >= TIMEOUT_MS){
			setup_peripheral();
			return 1;
		}
	}
	return 0;
}

static uint8_t wait_for_master_mode_selected(void) {
	uint32_t start_time;

	start_time = uptime_ms();
	while (
		!(
			(I2C_SR1(I2C) & I2C_SR1_SB)
			& (
				I2C_SR2(I2C) & (I2C_SR2_MSL | I2C_SR2_BUSY )
			)
		)
	) {
		if(uptime_ms() - start_time >= TIMEOUT_MS){
			setup_peripheral();
			return 1;
		}
	}
	return 0;
}

static uint8_t wait_for_address_transfer(void) {
	uint32_t start_time;

	start_time = uptime_ms();
	while (!(I2C_SR1(I2C) & I2C_SR1_ADDR)) {
		if(uptime_ms() - start_time >= TIMEOUT_MS){
			setup_peripheral();
			return 1;
		}
	}
	return 0;
}

uint8_t i2c_write(uint8_t addr, uint8_t *w, size_t wn) {
	if(wait_not_busy())
		return 1;

	i2c_send_start(I2C);

	if(wait_for_master_mode_selected())
		return 1;

	i2c_send_7bit_address(I2C, addr, I2C_WRITE);

	if(wait_for_address_transfer())
		return 1;

	/* Clearing ADDR condition sequence. */
	(void)I2C_SR2(I2C);

	for (size_t i = 0; i < wn; i++) {
		uint32_t start_time;

		i2c_send_data(I2C, w[i]);
		start_time = uptime_ms();
		while (
			!(
				I2C_SR1(I2C) & (I2C_SR1_BTF)
			)
		) {
			if(uptime_ms() - start_time >= TIMEOUT_MS){
				setup_peripheral();
				return 1;
			}
		}
	}

	i2c_send_stop(I2C);

	return 0;
}

uint8_t i2c_read(uint8_t addr, uint8_t *r, size_t rn) {
	i2c_send_start(I2C);
	i2c_enable_ack(I2C);

	if(wait_for_master_mode_selected())
		return 1;

	i2c_send_7bit_address(I2C, addr, I2C_READ);

	if(wait_for_address_transfer())
		return 1;

	/* Clearing ADDR condition sequence. */
	(void)I2C_SR2(I2C);

	for (size_t i = 0; i < rn; ++i) {
		uint32_t start_time;

		if (i == rn - 1) {
			i2c_disable_ack(I2C);
		}
		start_time = uptime_ms();
		while (!(I2C_SR1(I2C) & I2C_SR1_RxNE)) {
			if(uptime_ms() - start_time >= TIMEOUT_MS){
				setup_peripheral();
				return 1;
			}
		}
		r[i] = i2c_get_data(I2C);
	}
	i2c_send_stop(I2C);

	return 0;
}
