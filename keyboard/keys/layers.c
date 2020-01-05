#include "layers.h"
#include "../keys.h"
#include <libopencm3/usb/hid_usage_tables.h>

const uint8_t layers_default_state[] = {
	[LAYER_FN] = KEYS_LAYER_STATE_DISABLED,
	[LAYER_KEYPAD] = KEYS_LAYER_STATE_DISABLED,
	[LAYER_QWERTY_QWERTY] = KEYS_LAYER_STATE_CONFIG,
	[LAYER_QWERTY_DVORAK] = KEYS_LAYER_STATE_CONFIG,
	[LAYER_DVORAK_DVORAK] = KEYS_LAYER_STATE_CONFIG,
	[LAYER_DVORAK_QWERTY] = KEYS_LAYER_STATE_CONFIG,
	[LAYER_SHIFTED_NUMBER] = KEYS_LAYER_STATE_DISABLED,
	[LAYER_COMMON] = KEYS_LAYER_STATE_ENABLED,
};

// FIXME Implement No action
#define NONE { \
	.page=USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD, \
	.id=USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_A \
}

#define NEW_LAYER( \
	/* Left */ \
	k0x0, k0x1, k0x2, k0x3, k0x4, k0x5, k0x6, \
	k1x0, k1x1, k1x2, k1x3, k1x4, k1x5, k1x6, \
	k2x0, k2x1, k2x2, k2x3, k2x4, k2x5, \
	k3x0, k3x1, k3x2, k3x3, k3x4, k3x5, k2x6, \
	k4x0, k4x1, k4x2, k4x3, k4x4, k4x5, \
	k5x0, k5x1, k5x2,       k5x4,       k5x6, \
	k6x0, k6x1, k6x2,       k6x4,       k6x6, \
	/* Right */ \
	k0x7, k0x8, k0x9, k0x10, k0x11, k0x12, k0x13, k0x14, k0x15, \
	k1x7, k1x8, k1x9, k1x10, k1x11, k1x12, k1x13, k1x14, k1x15, \
	      k2x8, k2x9, k2x10, k2x11, k2x12, k2x13, k2x14, k2x15, \
	k2x7, k3x8, k3x9, k3x10, k3x11, k3x12, k3x13, k3x14, k3x15, \
	      k4x8, k4x9, k4x10, k4x11, k4x12, k4x13, k4x14, k4x15, \
	k5x7,       k5x9,        k5x11, k5x12, k5x13, k5x14, k5x15, \
	k6x7,       k6x9,        k6x11, k6x12, k6x13, k6x14, k6x15 \
) { \
	{k0x0, k0x1, k0x2, k0x3, k0x4, k0x5, k0x6,   k0x7, k0x8, k0x9, k0x10, k0x11, k0x12, k0x13, k0x14, k0x15}, \
	{k1x0, k1x1, k1x2, k1x3, k1x4, k1x5, k1x6,   k1x7, k1x8, k1x9, k1x10, k1x11, k1x12, k1x13, k1x14, k1x15}, \
	{k2x0, k2x1, k2x2, k2x3, k2x4, k2x5, k2x6,   k2x7, k2x8, k2x9, k2x10, k2x11, k2x12, k2x13, k2x14, k2x15}, \
	{k3x0, k3x1, k3x2, k3x3, k3x4, k3x5, NONE,   NONE, k3x8, k3x9, k3x10, k3x11, k3x12, k3x13, k3x14, k3x15}, \
	{k4x0, k4x1, k4x2, k4x3, k4x4, k4x5, NONE,   NONE, k4x8, k4x9, k4x10, k4x11, k4x12, k4x13, k4x14, k4x15}, \
	{k5x0, k5x1, k5x2, NONE, k5x4, NONE, k5x6,   k5x7, NONE, k5x9, NONE,  k5x11, k5x12, k5x13, k5x14, k5x15}, \
	{k6x0, k6x1, k6x2, NONE, k6x4, NONE, k6x6,   k6x7, NONE, k6x9, NONE,  k6x11, k6x12, k6x13, k6x14, k6x15}, \
}


#define ____ NONE // FIXME: next layer

// Keyboard/Keypad Usage Page
#define KBD(value) { \
	.page=USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD, \
	.id=USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_ ## value \
}
#define KPD(value) { \
	.page=USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD, \
	.id=USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYPAD_ ## value \
}

const struct keys_hid_usage_data layers_keymap[LAYER_COUNT][ROWS][COLUMNS] = {
	// [LAYER_FN] = NEW_LAYER(),
	// [LAYER_KEYPAD] = NEW_LAYER(),
	// [LAYER_QWERTY_QWERTY] = NEW_LAYER(),
	// [LAYER_QWERTY_DVORAK] = NEW_LAYER(),
	[LAYER_DVORAK_DVORAK] = NEW_LAYER(
		// Left
		____,                        ____,   ____,   ____,   ____,   ____,   ____,
		KBD(GRAVE_ACCENT_AND_TILDE), ____,   ____,   ____,   ____,   ____,   ____,
		____,                        KBD(Q), KBD(W), KBD(E), KBD(R), KBD(T),
		____,                        KBD(A), KBD(S), KBD(D), KBD(F), KBD(G), ____,
		____,                        KBD(Z), KBD(X), KBD(C), KBD(V), KBD(B),
		____,                        ____,   ____,           ____,           ____,
		____,                        ____,   ____,           ____,           ____,
		// Right
		____, ____,   ____,   ____,                          ____,                           ____,                         ____,                                   ____,                                   ____,
		____, ____,   ____,   ____,                          ____,                           ____,                         KBD(MINUS_AND_UNDERSCORE),              KBD(EQUAL_AND_PLUS),                    ____,
		      KBD(Y), KBD(U), KBD(I),                        KBD(O),                         KBD(P),                       KBD(OPENING_BRACKET_AND_OPENING_BRACE), KBD(CLOSING_BRACKET_AND_CLOSING_BRACE), ____,
		____, KBD(H), KBD(J), KBD(K),                        KBD(L),                         KBD(SEMICOLON_AND_COLON),     KBD(APOSTROPHE_AND_QUOTE),              ____,                                   ____,
		      KBD(N), KBD(M), KBD(COMMA_AND_LESS_THAN_SIGN), KBD(DOT_AND_GREATER_THAN_SIGN), KBD(SLASH_AND_QUESTION_MARK), KBD(BACKSLASH_AND_PIPE),                ____,                                   ____,
		____,         ____,                                  ____,                           ____,                         ____,                                   ____,                                   ____,
		____,         ____,                                  ____,                           ____,                         ____,                                   ____,                                   ____
	),
	// [LAYER_DVORAK_QWERTY] = NEW_LAYER(),
	// [LAYER_SHIFTED_NUMBER] = NEW_LAYER(),
	// [LAYER_COMMON] = NEW_LAYER(
	// 	// Left
	// 	____, ____, ____, ____, ____, ____, ____,
	// 	____, ____, ____, ____, ____, ____, ____,
	// 	____, ____, ____, ____, ____, ____,
	// 	____, ____, ____, ____, ____, ____, ____,
	// 	____, ____, ____, ____, ____, ____,
	// 	____, ____, ____,       ____,       ____,
	// 	____, ____, ____,       ____,       ____,
	// 	// Right
	// 	____, ____, ____, ____, ____, ____, ____, ____, ____,
	// 	____, ____, ____, ____, ____, ____, ____, ____, ____,
	// 	      ____, ____, ____, ____, ____, ____, ____, ____,
	// 	____, ____, ____, ____, ____, ____, ____, ____, ____,
	// 	      ____, ____, ____, ____, ____, ____, ____, ____,
	// 	____,       ____,       ____, ____, ____, ____, ____,
	// 	____,       ____,       ____, ____, ____, ____, ____
	// ),
};