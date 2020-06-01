#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>

#define MCP23017_BANK0_IOCON 0x0A
#define MCP23017_BANK1_IODIRA 0x00
#define MCP23017_BANK1_GPPUA 0x06
#define MCP23017_BANK1_GPIOA 0x09
#define MCP23017_BANK1_OLATA 0x0A
#define MCP23017_BANK1_IODIRB 0x10
#define MCP23017_BANK1_GPPUB 0x16
#define MCP23017_BANK1_GPIOB 0x19
#define MCP23017_BANK1_OLATB 0x1A

void mcp23017_write(uint8_t reg, uint8_t value);

void mcp23017_read(uint8_t reg, uint8_t *value);

#endif