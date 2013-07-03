/*
* Project: CatBoard (http://ibnteo.klava.org/tag/catboard)
* Version: 2.2
* Date: 2013-06-23
* Author: Vladimir Romanovich <ibnteo@gmail.com>
* License: GPL2
* 
* Based on: http://geekhack.org/index.php?topic=15542.0
* 
* Board: AVR-USB162MU (http://microsin.net/programming/AVR/avr-usb162mu.html) analogue Teensy 1.0
*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <util/delay.h>
#include "usb_keyboard.h"

#define STR_MANUFACTURER	L"ibnTeo"
#define STR_PRODUCT		L"CatBoard"

#define _PINC		(uint8_t *const)&PINC
#define _PORTC		(uint8_t *const)&PORTC
#define _PIND		(uint8_t *const)&PIND
#define _PORTD		(uint8_t *const)&PORTD
#define _PORTB		(uint8_t *const)&PORTB
#define _PINB		(uint8_t *const)&PINB

#define ROWS	5
#define COLS	12
#define KEYS	COLS*ROWS

#define NULL				0
#define NA					0
#define KEY_LAYER1			0xF1
#define KEY_LAYER2			0xF2
#define KEY_MY_SHIFT		0xF3
#define KEY_LED				0xF9
#define KEY_LOCK			0xFA
#define KEY_TURBO_REPEAT	0xFB
#define KEY_MAC_MODE		0xFC // (+Shift)
#define KEY_ALT_TAB			0xFD
#define KEY_FN_LOCK			0xFE
#define KEY_FN				0xFF
#define FN_KEY_ID			7*5+4
#define KEY_MOD				0x80
#define KEY_NULL			0

#define KEY_PRESSED_FN		1
#define KEY_PRESSED_MODS	2
#define KEY_PRESSED_ALT		3
#define KEY_PRESSED_SHIFT	4
#define KEY_PRESSED_CTRL	5
#define KEY_PRESSED_PREV	6

//#include "qwerty.h"
//#include "dvorak.h"
#include "jcuken.h"

//#include "at90usb162.h"
#include "at90usb162mu.h"

// 0 - shorcuts my layout; 1 - shorcuts qwerty layout
#define KEY_SHORTCUTS_LAYER1	1

// Nonstandart hardware layout
#define KEY_LAYOUT_ALT_SHIFT	1
#define KEY_LAYOUT_CTRL_SHIFT	2
#define KEY_LAYOUT_GUI_SPACE	3

//#define KEY_LAYOUT		0
#define KEY_LAYOUT		KEY_LAYOUT_ALT_SHIFT

// Mac mode off
uint8_t mac_mode = 0;

// 0x00-0x7F - normal keys
// 0x80-0xF0 - mod_keys | 0x80
// 0xF1-0xFF - catboard keys

const uint8_t layer1[KEYS] = {
	//ROW1			ROW2				ROW3			ROW4			ROW5
	KEY_TILDE,		KEY_TAB,			KEY_RIGHT_CTRL|KEY_MOD,KEY_ALT_TAB,KEY_ESC,				// COL1
	KEY_1,			KEY_Q,				KEY_A,			KEY_GUI|KEY_MOD,KEY_LAYER1,				// COL2
	KEY_2,			KEY_W,				KEY_S,			KEY_X,			KEY_Z,					// COL3
	KEY_3,			KEY_E,				KEY_D,			KEY_C,			KEY_LEFT_ALT|KEY_MOD,	// COL4
	KEY_4,			KEY_R,				KEY_F,			KEY_V,			KEY_LEFT_CTRL|KEY_MOD,	// COL5
	KEY_5,			KEY_T,				KEY_G,			KEY_B,			KEY_MY_SHIFT,			// COL6
	KEY_6,			KEY_Y,				KEY_H,			KEY_N,			KEY_SPACE,				// COL7 
	KEY_7,			KEY_U,				KEY_J,			KEY_M,			KEY_FN,					// COL8
	KEY_8,			KEY_I,				KEY_K,			KEY_COMMA,		KEY_RIGHT_ALT|KEY_MOD,	// COL9
	KEY_9,			KEY_O,				KEY_L,			KEY_PERIOD,		KEY_SLASH,				// COL10
	KEY_0,			KEY_P,				KEY_SEMICOLON,	KEY_BACKSLASH,	KEY_LAYER2,				// COL11
	KEY_MINUS,		KEY_LEFT_BRACE,		KEY_QUOTE,		KEY_RIGHT_BRACE,KEY_EQUAL				// COL12
};

/*
[e][~][1][2][3][4][5]  [6][7][8][9][0][-][=]
[a][t][Q][W][E][R][T]  [Y][U][I][O][P][b][b]
[m][c][A][S][D][F][G]  [H][J][K][L][:]["][\]
  [s] [Z][X][C][V][B]  [N][M][,][.][/] [s]
            [a][c][s]  [s][f][a]
r5c1 r1c1 r1c2 r1c3 r1c4 r1c5 r1c6      r1c7 r1c8 r1c9 r1c10 r1c11 r1c12 r5c12
r4c1 r2c1 r2c2 r2c3 r2c4 r2c5 r2c6      r2c7 r2c8 r2c9 r2c10 r2c11 r2c12 r4c12
r4c2 r3c1 r3c2 r3c3 r3c4 r3c5 r3c6      r3c7 r3c8 r3c9 r3c10 r3c11 r3c12 r4c11
  r5c2    r5c3 r4c3 r4c4 r4c5 r4c6      r4c7 r4c8 r4c9 r4c10 r5c10    r5c11
                    r5c4 r5c5   r5c6  r5c7   r5c8 r5c9
*/

// Start layout
uint8_t *layout = &layer2;

const uint8_t layer_fn[KEYS] = {
	//ROW1				ROW2			ROW3			ROW4			ROW5
	KEY_PRINTSCREEN,	KEY_TAB,		KEY_RIGHT_CTRL|KEY_MOD,KEY_ALT_TAB,KEY_TURBO_REPEAT,	// COL1
	KEY_F1,				NULL,			NULL,			KEY_GUI|KEY_MOD,KEY_LAYER1,				// COL2
	KEY_F2,				NULL,			NULL,			NULL,			NULL,					// COL3
	KEY_F3,				NULL,			NULL,			NULL,			KEY_LEFT_ALT|KEY_MOD,	// COL4
	KEY_F4,				NULL,			NULL,			NULL,			KEY_LEFT_CTRL|KEY_MOD,	// COL5
	KEY_F5,				KEY_TILDE,		NULL,			NULL,			KEY_MY_SHIFT,			// COL6
	KEY_F6,				KEY_LED,		KEY_ENTER,		KEY_BACKSPACE,	KEY_MAC_MODE,			// COL7
	KEY_F7,				KEY_HOME,		KEY_LEFT,		KEY_DELETE,		KEY_FN,					// COL8
	KEY_F8,				KEY_UP,			KEY_DOWN,		KEY_INSERT,		KEY_FN_LOCK,			// COL9
	KEY_F9,				KEY_END,		KEY_RIGHT,		NULL,			NULL,					// COL10
	KEY_F10,			KEY_PAGE_UP,	KEY_PAGE_DOWN,	NULL,			KEY_LOCK,				// COL11
	KEY_F11,			KEY_ESC,		KEY_PAUSE,		KEY_SCROLL_LOCK,KEY_F12					// COL12
};

const uint8_t layer_fnlock[KEYS] = {
	//ROW1				ROW2			ROW3			ROW4			ROW5
	KEY_TILDE,			KEY_TAB,		KEY_RIGHT_CTRL|KEY_MOD,KEY_ALT_TAB,KEY_TURBO_REPEAT,	// COL1
	KEY_1,				KEY_PAGE_UP,	KEY_PAGE_DOWN,	KEY_GUI|KEY_MOD,KEY_LAYER1,				// COL2
	KEY_2,				KEY_HOME,		KEY_LEFT,		NULL,			NULL,					// COL3
	KEY_3,				KEY_UP,			KEY_DOWN,		NULL,			KEY_LEFT_ALT|KEY_MOD,	// COL4
	KEY_4,				KEY_END,		KEY_RIGHT,		NULL,			KEY_LEFT_CTRL|KEY_MOD,	// COL5
	KEY_5,				KEY_TILDE,		KEY_ENTER,		NULL,			KEY_MY_SHIFT,			// COL6
	KEY_6,				KEYPAD_SLASH,	KEYPAD_ASTERIX,	KEYPAD_0,		KEY_SPACE,				// COL7
	KEY_7,				KEYPAD_7,		KEYPAD_4,		KEYPAD_1,		KEY_FN,					// COL8
	KEY_8,				KEYPAD_8,		KEYPAD_5,		KEYPAD_2,		KEY_RIGHT_ALT|KEY_MOD,	// COL9
	KEY_9,				KEYPAD_9,		KEYPAD_6,		KEYPAD_3,		NULL,					// COL10
	KEY_0,				KEYPAD_MINUS,	KEYPAD_PLUS,	KEYPAD_PERIOD,	KEY_LAYER2,				// COL11
	KEY_MINUS,			KEY_ESC,		KEY_ENTER,		KEY_NUM_LOCK,	KEY_EQUAL				// COL12
};

int8_t pressed[KEYS];
uint8_t queue[7] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t mod_keys = 0;
uint8_t *prev_layer = 0;

uint8_t turbo_repeat = 1;
uint8_t locked = 0;
uint8_t led = 1; // LED light

uint8_t last_key = 0xFF;
uint16_t press_time = 0;
uint16_t press_time2 = 0;
uint16_t release_time = 0;
uint16_t repeat_time = 0;

void init(void);
void send(void);
void poll(void);
void repeat_tick(void);
void key_press(uint8_t key_id);
void key_release(uint8_t key_id);
uint8_t get_code(uint8_t key_id);

int main(void) {
	// Disable watchdog if enabled by bootloader/fuses
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// Disable clock division
	clock_prescale_set(clock_div_1);

	init();
	for (;;) {
		poll();
	}
}

void init(void) {
	// Set for 16 MHz clock
	CLKPR = 0x80; CLKPR = 0;

	init_ports();

	LED_CONFIG;
	LED_RED_CONFIG;
	LED_BLUE_CONFIG;

	LED_RED_OFF;
	LED_BLUE_OFF;
	if (led) LED_ON;

	for (uint8_t i=0; i<KEYS; i++) {
		pressed[i] = 0;
	}

	usb_init();
	while(!usb_configured());
	LED_OFF;
	if (led) LED_RED_ON;
}

void poll() {
	uint8_t row, col, key_id;
	for (row=0; row<ROWS; row++) { // scan rows
		*row_port[row] &= ~row_bit[row];
		_delay_us(1);
		for (col=0; col<COLS; col++) { // read columns
			key_id = col*ROWS+row;
			if (! (*col_pin[col] & col_bit[col])) { // press key
				if (! pressed[key_id]) {
					key_press(key_id);
				}
			} else if (pressed[key_id]) { // release key
				key_release(key_id);
			}
		}
		*row_port[row] |= row_bit[row];
	}
	repeat_tick();
	_delay_ms(5);
}

void repeat_tick(void) {
	if (repeat_time) { // repeat pause
		if (repeat_time<(release_time>>2)) {
			repeat_time++;
		} else { // repeat press
			repeat_time = 1;
			if (turbo_repeat) {
				keyboard_modifier_keys = mod_keys;
				keyboard_keys[0] = get_code(last_key);
				if (! usb_keyboard_send()) { // repeat release
					keyboard_keys[0] = 0;
					usb_keyboard_send();
				}
			}
		}
	} else if (press_time2) { // press2 pause
		if (press_time2<(press_time+(pressed[FN_KEY_ID] ? 5 : 30))) {
			press_time2++;
		} else {
			repeat_time = 1;
		}
	} else if (release_time) { // release pause
		if (release_time<(press_time+50)) {
			release_time++;
		} else {
			last_key = 0xFF;
			release_time = 0;
			press_time = 0;
			press_time2 = 0;
			release_time = 0;
		}
	} else if (press_time) { // press1 pause
		if (press_time<250) {
			press_time++;
		} else {
			press_time = 0;
		}
	}	
}

void key_press(uint8_t key_id) {
	uint8_t i;
	uint8_t mods_pressed = (mod_keys & (KEY_CTRL|KEY_RIGHT_CTRL|KEY_ALT|KEY_RIGHT_ALT|KEY_GUI|KEY_RIGHT_GUI));
	pressed[key_id] = (pressed[FN_KEY_ID] ? KEY_PRESSED_FN : (mods_pressed ? KEY_PRESSED_MODS : -1));
	
	uint8_t key_code = ((pressed[key_id]==KEY_PRESSED_FN) ? layer_fn[key_id] : layout[key_id]);
	if (key_code==NULL) {
		key_code = layout[key_id];
		pressed[key_id] = (mods_pressed ? KEY_PRESSED_MODS : -1);
		if (key_code==NULL && prev_layer && ! mod_keys) {
			key_code = prev_layer[key_id];
			pressed[key_id] = KEY_PRESSED_PREV;
		}
	}

	if (locked && key_code!=KEY_LOCK) return;
	
	if (key_code>0xF0) { // Catboard keys
		if (key_code==KEY_ALT_TAB) { // AltTab press
			if (pressed[key_id]==KEY_PRESSED_FN) { // Fn + AltTab
				usb_keyboard_press(KEY_TAB, KEY_ALT);
			} else { // Alt press, Tab press and release
				if (! mod_keys) {
					mod_keys |= (KEY_ALT);
				} else {
					pressed[key_id] = KEY_PRESSED_ALT;
				}
				keyboard_modifier_keys = mod_keys;
				keyboard_keys[0] = KEY_TAB;
				usb_keyboard_send();
				_delay_ms(50);
				send();
			}
		} else if (key_code==KEY_FN_LOCK) { // FnLock
			if (prev_layer) { // FnLock Off
				layout = prev_layer;
				prev_layer = 0;
			} else { // FnLock On
				prev_layer = layout;
				layout = layer_fnlock;
			}
			if (prev_layer || mac_mode) {
				if (led) LED_ON;
			} else {
				LED_OFF;
			}
		} else if (key_code==KEY_MAC_MODE) { // Mac mode
			if (pressed[key_id]==KEY_PRESSED_FN && (mod_keys & (KEY_SHIFT|KEY_RIGHT_SHIFT))) {
				mac_mode = ! mac_mode;
				if (mac_mode || prev_layer) {
					if (led) LED_ON;
				} else {
					LED_OFF;
				}
			} else { // Press Space
				usb_keyboard_press(KEY_SPACE, mod_keys);
			}
		} else if (key_code==KEY_LAYER1) { // KEY_LAYOUT1
			if (mod_keys & (KEY_SHIFT|KEY_RIGHT_SHIFT)) {
				pressed[key_id] = KEY_PRESSED_CTRL;
				mod_keys |= KEY_CTRL;
				send();
			} else {
				if (mod_keys) pressed[key_id] = KEY_PRESSED_SHIFT;
				mod_keys |= KEY_SHIFT;
				send();
			}
		} else if (key_code==KEY_LAYER2) { // KEY_LAYOUT2
			mod_keys |= KEY_RIGHT_SHIFT;
			send();
		} else if (key_code==KEY_TURBO_REPEAT) { // TURBO_REPEAT ON/OFF
			turbo_repeat = ! turbo_repeat;
		} else if (key_code==KEY_MY_SHIFT) { // My Shift
			mod_keys |= KEY_SHIFT;
			send();
		} else if (key_code==KEY_LOCK) { // Lock/Unlock keyboard
			if (locked) {
				locked = 0;
				if (led) {
					if (layout==layer1 || prev_layer==layer1) LED_BLUE_ON;
					if (layout==layer2 || prev_layer==layer2) LED_RED_ON;
					if (prev_layer || mac_mode) LED_ON;
				}
			} else {
				locked = 1;
				LED_OFF;
				LED_RED_OFF;
				LED_BLUE_OFF;
				usb_keyboard_press(KEY_L, KEY_GUI); // Block computer
			}
		} else if (key_code==KEY_LED) { // LED On/Off
			if (led) {
				led = 0;
				LED_OFF;
				LED_RED_OFF;
				LED_BLUE_OFF;
			} else {
				led = 1;
				if (layout==layer1 || prev_layer==layer1) LED_BLUE_ON;
				if (layout==layer2 || prev_layer==layer2) LED_RED_ON;
				if (prev_layer || mac_mode) LED_ON;
			}
		}
	} else if (key_code>=0x80) { // Mod keys
		if (mac_mode && key_code==(KEY_CTRL|KEY_MOD)) {
			mod_keys |= KEY_GUI;
		} else if ((mac_mode && key_code==(KEY_RIGHT_CTRL|KEY_MOD)) || key_code==(KEY_RIGHT_GUI|KEY_MOD)) {
			mod_keys |= KEY_RIGHT_GUI;
		} else {
			mod_keys |= (key_code & 0x7F);
		}
		send();
	} else {
		if (! (last_key==key_id && release_time<10)) { // debounce
			for (i=5; i>0; i--) queue[i] = queue[i-1];
			queue[0] = key_id;
			send();
		}
	}
	// Autorepeat
	if (last_key==key_id) { // calc press2
		press_time2 = 1;
		repeat_time = 0;
	} else { // calc press1
		last_key = key_id;
		press_time = 1;
		press_time2 = 0;
		release_time = 0;
		repeat_time = 0;
	}
}

void key_release(uint8_t key_id) {
	uint8_t i;
	int8_t pressed_key_id = pressed[key_id];
	uint8_t key_code = ((pressed_key_id==KEY_PRESSED_FN) ? layer_fn[key_id] : layout[key_id]);
	if (pressed_key_id==KEY_PRESSED_PREV && prev_layer) {
		key_code = prev_layer[key_id];
	}
	pressed[key_id] = 0;
	if (locked) return;
	if (key_code>0xF0) { // Catboard keys release
		if (key_code==KEY_ALT_TAB && pressed_key_id!=KEY_PRESSED_ALT) { // AltTab: Alt release
			mod_keys &= ~(KEY_ALT);
			send();
		} else if (key_code==KEY_LAYER1 && pressed_key_id==KEY_PRESSED_CTRL) { // Layer1: Ctrl release
			mod_keys &= ~(KEY_CTRL);
			send();
		} else if (key_code==KEY_LAYER1) { // LAYER1
			mod_keys &= ~(KEY_SHIFT);
			send();
			if (last_key==key_id && press_time && press_time<50 && pressed_key_id!=KEY_PRESSED_SHIFT) {
				if (layout!=layer1) {
					if (layout==layer_fn) {
						prev_layer = layer1;
					} else {
						layout = layer1;
					}
					change_layout();
					//LED_ON;
					LED_RED_OFF;
					if (led) LED_BLUE_ON;
				}
			}
			last_key = 0xFF;
			press_time = 0;
			press_time2 = 0;
			release_time = 0;
			repeat_time = 0;
		} else if (key_code==KEY_LAYER2) { // LAYER2
			mod_keys &= ~(KEY_RIGHT_SHIFT);
			send();
			if (last_key==key_id && press_time && press_time<50 && pressed_key_id!=KEY_PRESSED_SHIFT) {
				if (layout!=layer2) {
					if (layout==layer_fn) {
						prev_layer = layer2;
					} else {
						layout = layer2;
					}
					change_layout();
					//LED_OFF;
					LED_BLUE_OFF;
					if (led) LED_RED_ON;
				}
			}
			last_key = 0xFF;
			press_time = 0;
			press_time2 = 0;
			release_time = 0;
			repeat_time = 0;
		} else if (key_code==KEY_MY_SHIFT) { // My Shift
			mod_keys &= ~KEY_SHIFT;
			send();
			if (last_key==key_id && press_time && press_time<50 && pressed_key_id!=KEY_PRESSED_MODS && ! mod_keys) {
				usb_keyboard_press(KEY_SPACE, mod_keys);
			}
			last_key = 0xFF;
			press_time = 0;
			press_time2 = 0;
			release_time = 0;
			repeat_time = 0;
		}
	} else if (key_code>=0x80) { // Mod keys release
		if (mac_mode && key_code==(KEY_CTRL|KEY_MOD)) {
			mod_keys &= ~KEY_GUI;
		} else if ((mac_mode && key_code==(KEY_RIGHT_CTRL|KEY_MOD)) || key_code==(KEY_RIGHT_GUI|KEY_MOD)) {
			mod_keys &= ~KEY_RIGHT_GUI;
		} else {
			mod_keys &= ~(key_code & 0x7F);
		}
		send();
	} else {
		for (i=0; i<6; i++) {
			if (queue[i]==key_id) {
				break;
			}
		}
		for (; i<6; i++) {
			queue[i] = queue[i+1];
		}
		send();
		// Autorepeat
		if (last_key==key_id) { // realise time
			press_time2 = 0;
			release_time = 1;
			repeat_time = 0;
		} else { // reset
			press_time = 0;
			press_time2 = 0;
			release_time = 0;
			repeat_time = 0;
		}
	}
}

void change_layout(void) {
	if (KEY_LAYOUT==KEY_LAYOUT_GUI_SPACE || mac_mode) { // Press Cmd+Space
		keyboard_modifier_keys = KEY_GUI;
		keyboard_keys[0] = 0;
		usb_keyboard_send();
		_delay_ms(50);
		usb_keyboard_press(KEY_SPACE, KEY_GUI);
	} else if (KEY_LAYOUT==KEY_LAYOUT_ALT_SHIFT) { // Press Alt+Shift
		keyboard_modifier_keys = KEY_ALT;
		keyboard_keys[0] = 0;
		usb_keyboard_send();
		_delay_ms(50);
		usb_keyboard_press(0, KEY_ALT|KEY_SHIFT);
	} else if (KEY_LAYOUT==KEY_LAYOUT_CTRL_SHIFT) { // Press Ctrl+Shift
		keyboard_modifier_keys = KEY_CTRL;
		keyboard_keys[0] = 0;
		usb_keyboard_send();
		_delay_ms(50);
		usb_keyboard_press(0, KEY_CTRL|KEY_SHIFT);
	}
}

void send(void) {
	uint8_t i;
	for (i=0; i<6; i++) {
		keyboard_keys[i] = get_code(queue[i]);
	}
	keyboard_modifier_keys = mod_keys;
	usb_keyboard_send();
}

uint8_t get_code(uint8_t key_id) {
	uint8_t key_code = 0;
	if (key_id<KEYS) { // not 0xFF
		if (pressed[key_id]==KEY_PRESSED_FN) { // key+Fn key
			if (layer_fn[key_id]>0 && layer_fn[key_id]<0x80) {
				key_code = layer_fn[key_id];
			}
		} else if (layout!=layer_fn && pressed[key_id]==KEY_PRESSED_MODS) { // keyboard shortcuts from layer1
			key_code = (KEY_SHORTCUTS_LAYER1 ? layer1[key_id] : layer2[key_id]);
		} else {
			key_code = layout[key_id];
		}
	}
	return key_code;
}
