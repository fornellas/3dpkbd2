#include "layers.h"
#include "../keys.h"
#include <libopencm3/usb/hid_usage_tables.h>

uint8_t layers_state[LAYER_COUNT];
static uint8_t layer_keypad_state = 0;
static uint32_t layout_changes_counter = 0;

void layout_set(uint16_t layout) {
	layout_changes_counter += 1;

	for(uint16_t layer_idx=LAYER_LAYOUT_START ; layer_idx <= LAYER_LAYOUT_END ; layer_idx++)
		layers_state[layer_idx] = (layer_idx == layout);
}

static uint8_t layout_get(void) {
	for(uint16_t layer_idx=LAYER_LAYOUT_START ; layer_idx <= LAYER_LAYOUT_END ; layer_idx++)
		if(layers_state[layer_idx])
			return layer_idx;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Macros
////////////////////////////////////////////////////////////////////////////////

#define KK(value) { \
	.page=USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD, \
	.id=USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_ ## value \
}

#define KBD(value) { \
	.page=USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD, \
	.id=USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYBOARD_ ## value \
}

#define KPD(value) { \
	.page=USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD, \
	.id=USB_HID_USAGE_PAGE_KEYBOARD_KEYPAD_KEYPAD_ ## value \
}

#define CSMR(value) { \
	.page=USB_HID_USAGE_PAGE_CONSUMER, \
	.id=USB_HID_USAGE_PAGE_CONSUMER_ ## value \
}

#define NONE { \
	.page=USB_HID_USAGE_PAGE_NONE, \
	.id=0 \
}

#define ____ { \
	.page=USB_HID_USAGE_PAGE_NEXT_LAYER, \
	.id=0 \
}

#define FUNC(value) { \
	.page=USB_HID_USAGE_PAGE_FUNCTION, \
	.id=FUNC_ ## value \
}

#define SEQ(value) { \
	.page=USB_HID_USAGE_PAGE_SEQUENCE, \
	.id=SEQ_ ## value \
}

#define SEQ_STEP(c, ...) { \
	.count = c, \
	.hid_usage = &(struct hid_usage_t[]) { \
		__VA_ARGS__ \
	}, \
}

#define LAYOUT(value) { \
	.page=USB_HID_USAGE_PAGE_LAYOUT, \
	.id=LAYER_ ## value \
}

#define SEQ_END SEQ_STEP(0)

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

static void func_cut(
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	struct hid_usage_list_t *
);

static void func_cut(
	uint8_t row,
	uint8_t column,
	uint8_t state,
	uint8_t pressed,
	uint8_t released,
	struct hid_usage_list_t *hid_usage_list
) {
	(void)row;
	(void)column;
	(void)state;
	(void)released;
	(void)hid_usage_list;

	if(pressed) {
		switch(layout_get()) {
			case LAYER_QWERTY_QWERTY:
			case LAYER_QWERTY_DVORAK:
				sequence_register(sequences[SEQ_CUT_QWERTY]);
				break;
			case LAYER_DVORAK_DVORAK:
			case LAYER_DVORAK_QWERTY:
				sequence_register(sequences[SEQ_CUT_DVORAK]);
				break;
		}
	}
};

static void func_copy(
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	struct hid_usage_list_t *
);

static void func_copy(
	uint8_t row,
	uint8_t column,
	uint8_t state,
	uint8_t pressed,
	uint8_t released,
	struct hid_usage_list_t *hid_usage_list
) {
	(void)row;
	(void)column;
	(void)state;
	(void)released;
	(void)hid_usage_list;

	if(pressed) {
		switch(layout_get()) {
			case LAYER_QWERTY_QWERTY:
			case LAYER_QWERTY_DVORAK:
				sequence_register(sequences[SEQ_COPY_QWERTY]);
				break;
			case LAYER_DVORAK_DVORAK:
			case LAYER_DVORAK_QWERTY:
				sequence_register(sequences[SEQ_COPY_DVORAK]);
				break;
		}
	}
};

static void func_paste(
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	struct hid_usage_list_t *
);

static void func_paste(
	uint8_t row,
	uint8_t column,
	uint8_t state,
	uint8_t pressed,
	uint8_t released,
	struct hid_usage_list_t *hid_usage_list
) {
	(void)row;
	(void)column;
	(void)state;
	(void)released;
	(void)hid_usage_list;

	if(pressed) {
		switch(layout_get()) {
			case LAYER_QWERTY_QWERTY:
			case LAYER_QWERTY_DVORAK:
				sequence_register(sequences[SEQ_PASTE_QWERTY]);
				break;
			case LAYER_DVORAK_DVORAK:
			case LAYER_DVORAK_QWERTY:
				sequence_register(sequences[SEQ_PASTE_DVORAK]);
				break;
		}
	}
};

static void func_fn(
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	struct hid_usage_list_t *
);

static void func_fn(
	uint8_t row,
	uint8_t column,
	uint8_t state,
	uint8_t pressed,
	uint8_t released,
	struct hid_usage_list_t *hid_usage_list
) {
	(void)row;
	(void)column;
	(void)state;
	(void)hid_usage_list;

	static uint8_t last_active_layout = 0;
	static uint32_t last_layout_changes_counter = 0;

	if(pressed) {
		uint8_t active_layout;
		uint8_t fn_alternative_layout;

		active_layout = layout_get();

		layers_state[LAYER_FN] = 1;
		layers_state[LAYER_KEYPAD] = 1;

		switch(active_layout){
			case LAYER_QWERTY_QWERTY:
			case LAYER_QWERTY_DVORAK:
				fn_alternative_layout = LAYER_QWERTY_QWERTY;
				break;
			case LAYER_DVORAK_DVORAK:
			case LAYER_DVORAK_QWERTY:
				fn_alternative_layout = LAYER_DVORAK_QWERTY;
				break;
			default:
				fn_alternative_layout = 0;
				break;
		}

		if(fn_alternative_layout)
			layout_set(fn_alternative_layout);

		last_active_layout = active_layout;
		last_layout_changes_counter = layout_changes_counter;
	}
	if(released) {
		layers_state[LAYER_FN] = 0;
		layers_state[LAYER_KEYPAD] = layer_keypad_state;

		if(layout_changes_counter == last_layout_changes_counter)
			if(last_active_layout)
				layout_set(last_active_layout);
	}
};

static void func_keypad(
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	struct hid_usage_list_t *
);

static void func_keypad(
	uint8_t row,
	uint8_t column,
	uint8_t state,
	uint8_t pressed,
	uint8_t released,
	struct hid_usage_list_t *hid_usage_list
) {
	(void)row;
	(void)column;
	(void)state;
	(void)released;
	(void)hid_usage_list;

	if(pressed) {
		layer_keypad_state = !layer_keypad_state;
		layers_state[LAYER_KEYPAD] = layer_keypad_state;
	}
};

static void func_shifted_number(
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	struct hid_usage_list_t *
);

static void func_shifted_number(
	uint8_t row,
	uint8_t column,
	uint8_t state,
	uint8_t pressed,
	uint8_t released,
	struct hid_usage_list_t *hid_usage_list
) {
	(void)row;
	(void)column;
	(void)state;
	(void)pressed;
	(void)released;
	(void)hid_usage_list;
	// TODO
};

static void func_toggle_shifted_number_layer(
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	struct hid_usage_list_t *
);

static void func_toggle_shifted_number_layer(
	uint8_t row,
	uint8_t column,
	uint8_t state,
	uint8_t pressed,
	uint8_t released,
	struct hid_usage_list_t *hid_usage_list
) {
	(void)row;
	(void)column;
	(void)state;
	(void)pressed;
	(void)released;
	(void)hid_usage_list;
	// TODO
};

void (* const functions[FUNC_COUNT])(
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	uint8_t,
	struct hid_usage_list_t *
) = {
  [FUNC_CUT] = &func_cut,
  [FUNC_COPY] = &func_copy,
  [FUNC_PASTE] = &func_paste,
  [FUNC_FN] = &func_fn,
  [FUNC_KEYPAD] = &func_keypad,
  [FUNC_SHIFTED_NUMBER] = &func_shifted_number,
  [FUNC_TOGGLE_SHIFTED_NUMBER_LAYER] = &func_toggle_shifted_number_layer,
};

////////////////////////////////////////////////////////////////////////////////
// Sequences
////////////////////////////////////////////////////////////////////////////////

static struct sequence_step_data seq_desktop_qwerty[] = {
	SEQ_STEP(2, KBD(LEFT_CONTROL), KBD(LEFT_ALT)),
	SEQ_STEP(3, KBD(LEFT_CONTROL), KBD(LEFT_ALT), KBD(D)),
	SEQ_END,
};

static struct sequence_step_data seq_desktop_dvorak[] = {
	SEQ_STEP(2, KBD(LEFT_CONTROL), KBD(LEFT_ALT)),
	SEQ_STEP(3, KBD(LEFT_CONTROL), KBD(LEFT_ALT), KBD(H)),
	SEQ_END,
};

static struct sequence_step_data seq_shuffle[] = {
	SEQ_STEP(1, KBD(LEFT_ALT)),
	SEQ_STEP(2, KBD(LEFT_ALT), KBD(TAB)),
	SEQ_END,
};

static struct sequence_step_data seq_00[] = {
	SEQ_STEP(1, KBD(0_AND_CLOSING_PARENTHESIS)),
	SEQ_STEP(1, KBD(0_AND_CLOSING_PARENTHESIS)),
	SEQ_END,
};

static struct sequence_step_data seq_b_tab[] = {
	SEQ_STEP(1, KBD(LEFT_SHIFT)),
	SEQ_STEP(2, KBD(LEFT_SHIFT), KBD(TAB)),
	SEQ_END,
};

static struct sequence_step_data seq_cut_qwerty[] = {
	SEQ_STEP(1, KBD(LEFT_CONTROL)),
	SEQ_STEP(2, KBD(LEFT_CONTROL), KBD(X)),
	SEQ_END,
};

static struct sequence_step_data seq_cut_dvorak[] = {
	SEQ_STEP(1, KBD(LEFT_CONTROL)),
	SEQ_STEP(2, KBD(LEFT_CONTROL), KBD(B)),
	SEQ_END,
};

static struct sequence_step_data seq_copy_qwerty[] = {
	SEQ_STEP(1, KBD(LEFT_CONTROL)),
	SEQ_STEP(2, KBD(LEFT_CONTROL), KBD(C)),
	SEQ_END,
};

static struct sequence_step_data seq_copy_dvorak[] = {
	SEQ_STEP(1, KBD(LEFT_CONTROL)),
	SEQ_STEP(2, KBD(LEFT_CONTROL), KBD(I)),
	SEQ_END,
};

static struct sequence_step_data seq_paste_qwerty[] = {
	SEQ_STEP(1, KBD(LEFT_CONTROL)),
	SEQ_STEP(2, KBD(LEFT_CONTROL), KBD(V)),
	SEQ_END,
};

static struct sequence_step_data seq_paste_dvorak[] = {
	SEQ_STEP(1, KBD(LEFT_CONTROL)),
	SEQ_STEP(2, KBD(LEFT_CONTROL), KBD(DOT_AND_GREATER_THAN_SIGN)),
	SEQ_END,
};


const struct sequence_step_data *sequences[SEQ_COUNT] = {
  [SEQ_DESKTOP_QWERTY] = seq_desktop_qwerty,
  [SEQ_DESKTOP_DVORAK] = seq_desktop_dvorak,
  [SEQ_SHUFFLE] = seq_shuffle,
  [SEQ_00] = seq_00,
  [SEQ_B_TAB] = seq_b_tab,
  [SEQ_CUT_QWERTY] = seq_cut_qwerty,
  [SEQ_CUT_DVORAK] = seq_cut_dvorak,
  [SEQ_COPY_QWERTY] = seq_copy_qwerty,
  [SEQ_COPY_DVORAK] = seq_copy_dvorak,
  [SEQ_PASTE_QWERTY] = seq_paste_qwerty,
  [SEQ_PASTE_DVORAK] = seq_paste_dvorak,
};

////////////////////////////////////////////////////////////////////////////////
// Layers
////////////////////////////////////////////////////////////////////////////////

const uint8_t layers_default_state[] = {
	[LAYER_FN] = KEYS_LAYER_STATE_DISABLED,
	[LAYER_KEYPAD] = KEYS_LAYER_STATE_DISABLED,
	[LAYER_QWERTY_QWERTY] = KEYS_LAYER_STATE_LOAD,
	[LAYER_QWERTY_DVORAK] = KEYS_LAYER_STATE_LOAD,
	[LAYER_DVORAK_DVORAK] = KEYS_LAYER_STATE_LOAD,
	[LAYER_DVORAK_QWERTY] = KEYS_LAYER_STATE_LOAD,
	[LAYER_SHIFTED_NUMBER] = KEYS_LAYER_STATE_DISABLED,
	[LAYER_COMMON] = KEYS_LAYER_STATE_ENABLED,
};

#define LAYER_KEYMAP( \
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

const struct hid_usage_t layers_keymap[LAYER_COUNT][ROWS][COLUMNS] = {
	[LAYER_FN] = LAYER_KEYMAP(
		// Left
		____, LAYOUT(QWERTY_QWERTY), LAYOUT(QWERTY_DVORAK), LAYOUT(DVORAK_DVORAK), LAYOUT(DVORAK_QWERTY), ____, KBD(INSERT),
		____, ____,                  ____,                  ____,                  ____,                  ____, CSMR(VOLUME_INCREMENT),
		____, ____,                  ____,                  ____,                  ____,                  ____,
		____, ____,                  ____,                  ____,                  ____,                  ____, CSMR(VOLUME_DECREMENT),
		____, ____,                  ____,                  ____,                  ____,                  ____,
		____, FUNC(CUT),             FUNC(COPY),                                   FUNC(PASTE),                 CSMR(MUTE),
		____, ____,                  ____,                                         ____,                        ____,
		// Right
		// FIXME Fn+F7 (power): TypeMatrix sends Monitor page 0x01
		// FIXME Fn+F8: (sleep): TypeMatrix sends Monitor page 0x02
		// FIXME Fn+F9 (wake): TypeMatrix sends Monitor page 0x03
		// TODO Fn+F10+Shift: SysReq
		// TODO Fn+F12+Shift: Break
		KBD(INSERT),            CSMR(EJECT),               KBD(POWER),                KK(SLEEP), ____, KBD(PRINT_SCREEN), KBD(SCROLL_LOCK), KBD(PAUSE), KPD(NUM_LOCK_AND_CLEAR),
		CSMR(VOLUME_INCREMENT), ____,                      ____,                      ____,      ____, ____,              ____,             ____,       ____,
		                        ____,                      ____,                      ____,      ____, ____,              ____,             ____,       ____,
		CSMR(VOLUME_DECREMENT), ____,                      ____,                      ____,      ____, ____,              ____,             ____,       FUNC(TOGGLE_SHIFTED_NUMBER_LAYER),
		                        ____,                      ____,                      ____,      ____, ____,              ____,             ____,       ____,
		CSMR(MUTE),                                        CSMR(SCAN_PREVIOUS_TRACK),            ____, ____,              ____,             ____,       CSMR(AC_BACK),
		____,                                              CSMR(SCAN_NEXT_TRACK),                ____, ____,              ____,             ____,       CSMR(AC_FORWARD)
	),
	[LAYER_KEYPAD] = LAYER_KEYMAP(
		// Left
		____, ____, ____, ____, ____, ____, ____,
		____, ____, ____, ____, ____, ____, ____,
		____, ____, ____, ____, ____, ____,
		____, ____, ____, ____, ____, ____, ____,
		____, ____, ____, ____, ____, ____,
		____, ____, ____,       ____,       ____,
		____, ____, ____,       ____,       ____,
		// Right
		____, ____,            ____,            ____,             ____,                  ____,                  ____,                   ____,       ____,
		____, ____,            ____,            ____,             ____,                  ____,                  ____,                   ____,       ____,
		      ____,            ____,            ____,             KBD(TAB),              KPD(SLASH),            KPD(ASTERISK),          KPD(MINUS), SEQ(B_TAB),
		____, KBD(HOME),       KBD(UP_ARROW),   KBD(END),         KPD(7_AND_HOME),       KPD(8_AND_UP_ARROW),   KPD(9_AND_PAGE_UP),     KPD(PLUS),  KBD(ESCAPE),
		      KBD(LEFT_ARROW), KBD(DOWN_ARROW), KBD(RIGHT_ARROW), KPD(4_AND_LEFT_ARROW), KPD(5),                KPD(6_AND_RIGHT_ARROW), ____,       KBD(DELETE_BACKSPACE),
		____,                  ____,                              KPD(1_AND_END),        KPD(2_AND_DOWN_ARROW), KPD(3_AND_PAGE_DOWN),   KPD(ENTER), ____,
		____,                  ____,                              KPD(0_AND_INSERT),     SEQ(00),               KPD(DOT_AND_DELETE),    ____,       ____
	),
	[LAYER_QWERTY_QWERTY] = LAYER_KEYMAP(
		// Left
		____,                        ____,   ____,                ____,   ____,   ____,   ____,
		KBD(GRAVE_ACCENT_AND_TILDE), ____,   ____,                ____,   ____,   ____,   ____,
		____,                        KBD(Q), KBD(W),              KBD(E), KBD(R), KBD(T),
		____,                        KBD(A), KBD(S),              KBD(D), KBD(F), KBD(G), ____,
		____,                        KBD(Z), KBD(X),              KBD(C), KBD(V), KBD(B),
		____,                        ____,   ____,                        ____,           ____,
		____,                        ____,   SEQ(DESKTOP_QWERTY),         ____,           ____,
		// Right
		____, ____,   ____,   ____,                          ____,                           ____,                         ____,                                   ____,                                   ____,
		____, ____,   ____,   ____,                          ____,                           ____,                         KBD(MINUS_AND_UNDERSCORE),              KBD(EQUAL_AND_PLUS),                    ____,
		      KBD(Y), KBD(U), KBD(I),                        KBD(O),                         KBD(P),                       KBD(OPENING_BRACKET_AND_OPENING_BRACE), KBD(CLOSING_BRACKET_AND_CLOSING_BRACE), ____,
		____, KBD(H), KBD(J), KBD(K),                        KBD(L),                         KBD(SEMICOLON_AND_COLON),     KBD(APOSTROPHE_AND_QUOTE),              ____,                                   ____,
		      KBD(N), KBD(M), KBD(COMMA_AND_LESS_THAN_SIGN), KBD(DOT_AND_GREATER_THAN_SIGN), KBD(SLASH_AND_QUESTION_MARK), KBD(BACKSLASH_AND_PIPE),                ____,                                   ____,
		____,         ____,                                  ____,                           ____,                         ____,                                   ____,                                   ____,
		____,         ____,                                  ____,                           ____,                         ____,                                   ____,                                   ____
	),
	[LAYER_QWERTY_DVORAK] = LAYER_KEYMAP(
		// Left
		____,                        ____,                      ____,                          ____,                           ____,   ____,   ____,
		KBD(GRAVE_ACCENT_AND_TILDE), ____,                      ____,                          ____,                           ____,   ____,   ____,
		____,                        KBD(APOSTROPHE_AND_QUOTE), KBD(COMMA_AND_LESS_THAN_SIGN), KBD(DOT_AND_GREATER_THAN_SIGN), KBD(P), KBD(Y),
		____,                        KBD(A),                    KBD(O),                        KBD(E),                         KBD(U), KBD(I), ____,
		____,                        KBD(SEMICOLON_AND_COLON),  KBD(Q),                        KBD(J),                         KBD(K), KBD(X),
		____,                        ____,                      ____,                          ____,                                   ____,
		____,                        ____,                      SEQ(DESKTOP_QWERTY),           ____,                                   ____,
		// Right
		____, ____,   ____,   ____,   ____,   ____,   ____,                                   ____,                                   ____,
		____, ____,   ____,   ____,   ____,   ____,   KBD(OPENING_BRACKET_AND_OPENING_BRACE), KBD(CLOSING_BRACKET_AND_CLOSING_BRACE), ____,
		      KBD(F), KBD(G), KBD(C), KBD(R), KBD(L), KBD(SLASH_AND_QUESTION_MARK),           KBD(EQUAL_AND_PLUS),                    ____,
		____, KBD(D), KBD(H), KBD(T), KBD(N), KBD(S), KBD(MINUS_AND_UNDERSCORE),              ____,                                   ____,
		      KBD(B), KBD(M), KBD(W), KBD(V), KBD(Z), KBD(BACKSLASH_AND_PIPE),                ____,                                   ____,
		____,         ____,           ____,   ____,   ____,                                   ____,                                   ____,
		____,         ____,           ____,   ____,   ____,                                   ____,                                   ____
	),
	[LAYER_DVORAK_DVORAK] = LAYER_KEYMAP(
		// Left
		____,                        ____,   ____,                ____,   ____,   ____,   ____,
		KBD(GRAVE_ACCENT_AND_TILDE), ____,   ____,                ____,   ____,   ____,   ____,
		____,                        KBD(Q), KBD(W),              KBD(E), KBD(R), KBD(T),
		____,                        KBD(A), KBD(S),              KBD(D), KBD(F), KBD(G), ____,
		____,                        KBD(Z), KBD(X),              KBD(C), KBD(V), KBD(B),
		____,                        ____,   ____,                        ____,           ____,
		____,                        ____,   SEQ(DESKTOP_DVORAK),         ____,           ____,
		// Right
		____, ____,   ____,   ____,                          ____,                           ____,                         ____,                                   ____,                                   ____,
		____, ____,   ____,   ____,                          ____,                           ____,                         KBD(MINUS_AND_UNDERSCORE),              KBD(EQUAL_AND_PLUS),                    ____,
		      KBD(Y), KBD(U), KBD(I),                        KBD(O),                         KBD(P),                       KBD(OPENING_BRACKET_AND_OPENING_BRACE), KBD(CLOSING_BRACKET_AND_CLOSING_BRACE), ____,
		____, KBD(H), KBD(J), KBD(K),                        KBD(L),                         KBD(SEMICOLON_AND_COLON),     KBD(APOSTROPHE_AND_QUOTE),              ____,                                   ____,
		      KBD(N), KBD(M), KBD(COMMA_AND_LESS_THAN_SIGN), KBD(DOT_AND_GREATER_THAN_SIGN), KBD(SLASH_AND_QUESTION_MARK), KBD(BACKSLASH_AND_PIPE),                ____,                                   ____,
		____,         ____,                                  ____,                           ____,                         ____,                                   ____,                                   ____,
		____,         ____,                                  ____,                           ____,                         ____,                                   ____,                                   ____
	),
	[LAYER_DVORAK_QWERTY] = LAYER_KEYMAP(
		// Left
		____,                        ____,                         ____,                          ____,   ____,                           ____,   ____,
		KBD(GRAVE_ACCENT_AND_TILDE), ____,                         ____,                          ____,   ____,                           ____,   ____,
		____,                        KBD(X),                       KBD(COMMA_AND_LESS_THAN_SIGN), KBD(D), KBD(O),                         KBD(K),
		____,                        KBD(A),                       KBD(SEMICOLON_AND_COLON),      KBD(H), KBD(Y),                         KBD(U), ____,
		____,                        KBD(SLASH_AND_QUESTION_MARK), KBD(B),                        KBD(I), KBD(DOT_AND_GREATER_THAN_SIGN), KBD(N),
		____,                        ____,                         ____,                          ____,                                   ____,
		____,                        ____,                         SEQ(DESKTOP_DVORAK),           ____,                                   ____,
		// Right
		____, ____,   ____,   ____,   ____,   ____,                                   ____,                      ____,                                   ____,
		____, ____,   ____,   ____,   ____,   ____,                                   KBD(APOSTROPHE_AND_QUOTE), KBD(CLOSING_BRACKET_AND_CLOSING_BRACE), ____,
		      KBD(T), KBD(F), KBD(G), KBD(S), KBD(R),                                 KBD(MINUS_AND_UNDERSCORE), KBD(EQUAL_AND_PLUS),                    ____,
		____, KBD(J), KBD(C), KBD(V), KBD(P), KBD(Z),                                 KBD(Q),                    ____,                                   ____,
		      KBD(P), KBD(M), KBD(W), KBD(E), KBD(OPENING_BRACKET_AND_OPENING_BRACE), KBD(BACKSLASH_AND_PIPE),   ____,                                   ____,
		____,         ____,           ____,   ____,                                   ____,                      ____,                                   ____,
		____,         ____,           ____,   ____,                                   ____,                      ____,                                   ____
	),
	[LAYER_SHIFTED_NUMBER] = LAYER_KEYMAP(
		// Left
		____, ____,                 ____,                 ____,                 ____,                 ____,                 ____,
		____, FUNC(SHIFTED_NUMBER), FUNC(SHIFTED_NUMBER), FUNC(SHIFTED_NUMBER), FUNC(SHIFTED_NUMBER), FUNC(SHIFTED_NUMBER), ____,
		____, ____,                 ____,                 ____,                 ____,                 ____,
		____, ____,                 ____,                 ____,                 ____,                 ____,                 ____,
		____, ____,                 ____,                 ____,                 ____,                 ____,
		____, ____,                 ____,                                       ____,                                       ____,
		____, ____,                 ____,                                       ____,                                       ____,
		// Right
		____, ____,                 ____,                 ____,                 ____,                 ____,                 ____, ____, ____,
		____, FUNC(SHIFTED_NUMBER), FUNC(SHIFTED_NUMBER), FUNC(SHIFTED_NUMBER), FUNC(SHIFTED_NUMBER), FUNC(SHIFTED_NUMBER), ____, ____, ____,
		      ____,                 ____,                 ____,                 ____,                 ____,                 ____, ____, ____,
		____, ____,                 ____,                 ____,                 ____,                 ____,                 ____, ____, ____,
		      ____,                 ____,                 ____,                 ____,                 ____,                 ____, ____, ____,
		____,                       ____,                                       ____,                 ____,                 ____, ____, ____,
		____,                       ____,                                       ____,                 ____,                 ____, ____, ____
	),
	[LAYER_COMMON] = LAYER_KEYMAP(
		// Left
		KBD(ESCAPE),      KBD(F1),                KBD(F2),          KBD(F3),             KBD(F4),           KBD(F5),               KBD(DELETE_FORWARD),
		____,             KBD(1_AND_EXCLAMATION), KBD(2_AND_AT),    KBD(3_AND_HASHMARK), KBD(4_AND_DOLLAR), KBD(5_AND_PERCENTAGE), KBD(DELETE_BACKSPACE),
		KBD(TAB),         ____,                   ____,             ____,                ____,              ____,
		____,             ____,                   ____,             ____,                ____,              ____,                  KBD(RETURN_ENTER),
		____,             ____,                   ____,             ____,                ____,              ____,
		CSMR(PLAY_PAUSE), KBD(APPLICATION),       SEQ(SHUFFLE),                          KBD(LEFT_SHIFT),                          KBD(SPACEBAR),
		FUNC(FN),         KBD(LEFT_GUI),          ____,                                  KBD(LEFT_CONTROL),                        KBD(LEFT_ALT),
		// Right
		KBD(DELETE_FORWARD),   KBD(F6),          KBD(F7),              KBD(F8),             KBD(F9),                        KBD(F10),                       KBD(F11),         KBD(F12),           FUNC(KEYPAD),
		KBD(DELETE_BACKSPACE), KBD(6_AND_CARET), KBD(7_AND_AMPERSAND), KBD(8_AND_ASTERISK), KBD(9_AND_OPENING_PARENTHESIS), KBD(0_AND_CLOSING_PARENTHESIS), ____,             ____,               CSMR(AL_CALCULATOR),
		                       ____,             ____,                 ____,                ____,                           ____,                           ____,             ____,               CSMR(AL_EMAIL_READER),
		KBD(RETURN_ENTER),     ____,             ____,                 ____,                ____,                           ____,                           ____,             ____,               KBD(CAPS_LOCK),
		                       ____,             ____,                 ____,                ____,                           ____,                           ____,             ____,               CSMR(AC_HOME),
		KBD(SPACEBAR),                           KBD(RIGHT_SHIFT),                          KBD(HOME),                      KBD(UP_ARROW),                  KBD(END),         ____,               KBD(PAGE_UP),
		KBD(RIGHT_ALT),                          KBD(RIGHT_CONTROL),                        KBD(LEFT_ARROW),                KBD(DOWN_ARROW),                KBD(RIGHT_ARROW), ____,               KBD(PAGE_DOWN)
	),
};