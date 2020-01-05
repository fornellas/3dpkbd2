#include "descriptors.h"
#include <stdint.h>

void keys_setup(void);

void keys_reset(void);

void keys_populate_hid_in_report(struct hid_in_report_data *);

void keys_scan(void (*)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, void *), void *);