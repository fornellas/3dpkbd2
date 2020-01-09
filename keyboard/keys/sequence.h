#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "../keys.h"

struct sequence_step_data {
	uint8_t count;
	struct keys_hid_usage_data (*hid_usage)[];
};

void sequence_init(void);
void sequence_register(const struct sequence_step_data[]);
void sequence_play(struct hid_in_report_data *);

#endif