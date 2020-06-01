#include "i2c.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>

#define RCC_GPIO_I2C RCC_GPIOB
#define RCC_I2C RCC_I2C1
#define GPIO_I2C GPIOB

#define GPIO_SCL GPIO8
#define GPIO_SDA GPIO9

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