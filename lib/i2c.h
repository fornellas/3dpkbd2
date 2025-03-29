#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdio.h>

#define I2C I2C1
#define RST_I2C RST_I2C1

void i2c_setup(void);
uint8_t i2c_write(uint8_t addr, uint8_t *data, size_t len);
uint8_t i2c_read(uint8_t addr, uint8_t *data, size_t len);

#endif