#include "sequence.h"
#include "../descriptors.h"
#include <stdlib.h>
#include <string.h>

static struct sequence_step_data *active_sequence;
static uint8_t active_sequence_step;

void sequence_init(void) {
	active_sequence = NULL;
	active_sequence_step = 0;
}

static size_t get_length(const struct sequence_step_data *sequence) {
  size_t len;

  len = 1;
  for(size_t i=0 ;  ; i++)
    if(sequence[i].count)
      len++;
    else
      break;
  return len;
}

void sequence_register(const struct sequence_step_data new_sequence[]) {
	size_t size;

	if(active_sequence != NULL)
		return;

	size = sizeof(struct sequence_step_data) * get_length(new_sequence);
	active_sequence = malloc(size);
	memcpy(active_sequence, new_sequence, (size_t)size);
	active_sequence_step = 0;
}

void sequence_play(struct hid_usage_list_t *hid_usage_list) {
	struct sequence_step_data *sequence_step;

	if(active_sequence == NULL)
		return;

	sequence_step = &active_sequence[active_sequence_step];

	if(sequence_step->count) {
		for(uint8_t i=0 ; i < sequence_step->count ; i++) {
			struct hid_usage_t *hid_usage;

			hid_usage = &((*sequence_step->hid_usage)[i]);
			hid_usage_list_add(hid_usage_list, hid_usage->page, hid_usage->id);
		}
		active_sequence_step++;
	} else {
		free(active_sequence);
		active_sequence = NULL;
		active_sequence_step = 0;
	}
}

void sequence_reset(void) {
	if(active_sequence) {
		free(active_sequence);
		active_sequence = NULL;
		active_sequence_step = 0;
	}
}
