#include "scan.h"
#include "sequence.h"
#include "../keys.h"

enum function_defs {
  FUNC_FN,
  FUNC_KEYPAD,
  FUNC_COMMON_SHIFTED,
  FUNC_TOGGLE_SHIFTED_NUMBER_LAYER,
  FUNC_CUT,
  FUNC_COPY,
  FUNC_PASTE,
  FUNC_DESKTOP,
  FUNC_COUNT,
};

enum sequence_defs {
  SEQ_SHUFFLE,
  SEQ_00,
  SEQ_B_TAB,
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

extern const uint8_t layers_default_state[LAYER_COUNT];
extern const struct keys_hid_usage_data layers_keymap[LAYER_COUNT][ROWS][COLUMNS];