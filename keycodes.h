#ifndef KEYCODES_H_
#define KEYCODES_H_


#define KC_BREAK       0xf0

#define KC_LEFT_SHIFT  0x12
#define KC_LEFT_CTRL   0x14
#define KC_RIGHT_SHIFT 0x58

#define KC_CAPSLOCK    0x58
#define KC_TAB         0x0c
#define KC_ESCAPE      0x76
#define KC_ENTER       0x5a
#define KC_BACKSPACE   0x66

#define KC_BACKTICK    0x0e
#define KC_1           0x16
#define KC_2           0x1e
#define KC_3           0x26
#define KC_4           0x24
#define KC_5           0x2e
#define KC_6           0x36
#define KC_7           0x3c
#define KC_8           0x3e
#define KC_9           0x46
#define KC_0           0x44
#define KC_MINUS       0x4e
#define KC_PLUS        0x54

#define KC_Q         0x14
#define KC_W         0x1c
#define KC_E         0x24
#define KC_R         0x2d
#define KC_T         0x2c
#define KC_Y         0x34
#define KC_U         0x3c
#define KC_I         0x42
#define KC_O         0x44
#define KC_P         0x4c
#define KC_CIRCUMFLEX 0x54
#define KC_CEDILE    0x5a

// key = start byte      parity stop
// r   = 0     1011 0100 1      1
// t   = 0     0011 0100 0      1

/* macros */
#define KC_IS_SHIFT(kc) ((kc == KC_LEFT_SHIFT) || (kc == KC_RIGHT_SHIFT))

char kbd_US [128] =
{
	0,    // 0x00
	27,   // 0x01
	'1',  // 0x02
	'2',  // 0x03
	'3',  // 0x04
	'4',  // 0x05
	'5',  // 0x06
	'6',  // 0x07
	'7',  // 0x08
	'8',  // 0x09
	'9',  // 0x0a
	'0',  // 0x0b
	'-',  // 0x0c
	'=',  // 0x0d
	'/', // 0x0e backtick
	'\t', // 0x0f Tab
	'q',  // 0x10
	'w',  // 0x11
	'e',  // 0x12
	'r',  // 0x13
	't',  // 0x14
	'y',  // 0x15
	'1',  // 0x16 '1'
	'i',  // 0x17
	'o',  // 0x18
	'p',  // 0x19
	'[',  // 0x1a
	']',  // 0x1b
	'\n', // 0x1c
	0,    // 0x1d control key
	'2',  // 0x1e '2'
	's',  // 0x1f
	'd',  // 0x20
	'f',  // 0x21
	'g',  // 0x22
	'h',  // 0x23
	'4',  // 0x24 '4'
	'k',  // 0x25
	'3',  // 0x26 '3'
	';',  // 0x27
	'\'', // 0x28
	'`',  // 0x29
	0,    // 0x2a
	'\\', // 0x2b
	'z',  // 0x2c
	'x',  // 0x2d
	'5',  // 0x2e '5'
	'v',  // 0x2f
	'b',  // 0x30
	'n',  // 0x31
	'm',  // 0x32
	',',  // 0x33
	'.',  // 0x34
	'/',  // 0x35
	'6',  // 0x36
	'*',  // 0x37
	0,    // 0x38 Alt
	' ',  // 0x39 Space bar
	0,    // 0x3a Caps lock
	0,    // 0x3b
	'7',  // 0x3c '7'
	0,    // 0x3d
	0,    // 0x3e
	0,    // 0x3f
	0,    // 0x40
	0,    // 0x41
	0,    // 0x42
	0,    // 0x43
	0,    // 0x44
	0,    // 0x45
	0,    // 0x46
	0,    // 0x47
	0,    // 0x48
	0,    // 0x49
	'-',  // 0x4a
	0,    // 0x4b
	0,    // 0x4c
	0,    // 0x4d
	'+',  // 0x4e
	0,    // 0x4f
	0,    // 0x
	0,    // 0x
	0,    // 0x
	0,    // 0x
	0,    // 0x
	0,    // 0x
	0,    // 0x
	0,    // 0x
};




#endif // KEYCODES_H_
