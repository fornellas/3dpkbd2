#include "mcp23017.h"
#include "lib/i2c.h"
#include <libopencm3/stm32/i2c.h>

#define MCP23017_ADDRESS 0x27

uint8_t mcp23017_write(uint8_t reg, uint8_t value) {
	uint8_t data[2];

	data[0] = reg;
	data[1] = value;
	return i2c_write(MCP23017_ADDRESS, data, 2);
}

uint8_t mcp23017_read(uint8_t reg, uint8_t *value) {
	if(i2c_write(MCP23017_ADDRESS, &reg, 1))
		return 1;
	if(i2c_read(MCP23017_ADDRESS, value, 1))
		return 1;
	return 0;
}