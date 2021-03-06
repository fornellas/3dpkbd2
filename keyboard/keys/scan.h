#ifndef KEYS_SCAN_H
#define KEYS_SCAN_H

#include <stdint.h>

#define ROWS 7
#define LEFT_COLUMNS 7
#define RIGHT_COLUMNS 9
#define COLUMNS (LEFT_COLUMNS + RIGHT_COLUMNS)

extern uint8_t keys_scan_right_side_disconnected;
extern uint32_t last_key_trigger_ms;

void keys_scan_setup(void);
void keys_scan_state_reset(void);

#endif