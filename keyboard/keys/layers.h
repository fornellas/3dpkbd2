#include "scan.h"
#include "sequence.h"
#include "../keys.h"
#include "../hid.h"
#include "../descriptors.h"

enum function_defs {
  FUNC_CUT,
  FUNC_COPY,
  FUNC_PASTE,
  FUNC_FN,
  FUNC_KEYPAD,
  FUNC_COUNT,
};

extern void (* const functions[FUNC_COUNT])(
  uint8_t,
  uint8_t,
  uint8_t,
  uint8_t,
  uint8_t,
  struct hid_usage_list_t *
);

enum sequence_defs {
  SEQ_DESKTOP_QWERTY,
  SEQ_DESKTOP_DVORAK,
  SEQ_SHUFFLE,
  SEQ_00,
  SEQ_B_TAB,
  SEQ_CUT_QWERTY,
  SEQ_CUT_DVORAK,
  SEQ_COPY_QWERTY,
  SEQ_COPY_DVORAK,
  SEQ_PASTE_QWERTY,
  SEQ_PASTE_DVORAK,
  SEQ_ALT_1,
  SEQ_ALT_2,
  SEQ_ALT_3,
  SEQ_ALT_4,
  SEQ_COUNT,
};

extern const struct sequence_step_data *sequences[SEQ_COUNT];

enum layers {
  LAYER_FN,
  LAYER_KEYPAD,
  LAYER_QWERTY_QWERTY,
  LAYER_QWERTY_DVORAK,
  LAYER_DVORAK_DVORAK,
  LAYER_DVORAK_QWERTY,
  LAYER_SHIFTED_NUMBER,
  LAYER_COMMON,
  LAYER_COUNT,
};

#define LAYER_LAYOUT_START LAYER_QWERTY_QWERTY
#define LAYER_LAYOUT_END LAYER_DVORAK_QWERTY

extern uint8_t layers_state[LAYER_COUNT];
extern const uint8_t layers_default_state[LAYER_COUNT];
extern const struct hid_usage_t layers_keymap[LAYER_COUNT][ROWS][COLUMNS];

void layout_set(uint16_t layout);
void toggle_layer(enum layers layer);