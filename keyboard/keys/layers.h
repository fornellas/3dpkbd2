#include "scan.h"
#include "../keys.h"

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