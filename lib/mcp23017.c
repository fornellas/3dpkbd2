#include "mcp23017.h"
#include "lib/i2c.h"
#include <libopencm3/stm32/i2c.h>

#define MCP23017_ADDRESS 0x27

void mcp23017_write(uint8_t reg, uint8_t value) {
	uint8_t data[2];

	data[0] = reg;
	data[1] = value;
	i2c_transfer7(I2C, MCP23017_ADDRESS, data, 2, NULL, 0);
}

void mcp23017_read(uint8_t reg, uint8_t *value) {
	i2c_transfer7(I2C, MCP23017_ADDRESS, &reg, 1, NULL, 0);
	i2c_transfer7(I2C, MCP23017_ADDRESS, NULL, 0, value, 1);
}