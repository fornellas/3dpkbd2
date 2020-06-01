#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdio.h>

#define I2C I2C1

void i2c_setup(void);
uint8_t i2c_write(uint8_t addr, uint8_t *w, size_t wn);
uint8_t i2c_read(uint8_t addr, uint8_t *r, size_t rn);

#endif