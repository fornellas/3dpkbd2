#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void systick_setup(void);

void delay_ms(uint32_t);

uint32_t uptime_ms(void);

#endif