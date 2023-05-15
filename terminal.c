#include "terminal.h"

#include "keycodes.h"
#include "common.h"
#include "font.h"

#include "bsp/board.h"

#include "pico/stdlib.h"

#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/uart.h"

#include <stdio.h>
#include <string.h>


#define DEBUG_BUF_LEN 256

// not sure what that does
//#define USE_ANSI_ESCAPE 0

#define MAX_REPORT 4


/* need those from cvideo.c */
//extern const int WHITE;
//extern const int BLACK;

/* private variables declarations */
/* position in pixel array */
static int posx = gapx;
static int posy = gapy;

/* ring buffer struct for ps/2 keyboard keycodes */
#define KEYCODES_MAX_NUM 64
struct {
	int iread;
	int iwrite;
	char buf[KEYCODES_MAX_NUM];
} keycodes_ringbuf;


/* private functions declarations */
static void fs(void);
static void bs(void);
static void lf(void);
static void scroll_up(int rows);
static void print_char(char c);
static void process_kbd_report(hid_keyboard_report_t const *report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance,
                                    uint8_t const *report, uint16_t len);
static void ps2_keycode_callback(uint gpio, uint32_t events);
static int keycodes_ringbuf_put(char kc);
static int keycodes_ringbuf_get(char *kc);
static int keycodes_ringbuf_inc(int idx);
static int keycodes_ringbuf_dec(int idx);
static void handle_keycode(char kc);


/* public functions definitions */

void
terminal_init(void)
{
	uart_init(uart0, 115200);
	gpio_set_function(12, GPIO_FUNC_UART);  // TX
	gpio_set_function(13, GPIO_FUNC_UART);  // RX

	/* interrupt pin */
	gpio_set_irq_enabled_with_callback(16, GPIO_IRQ_EDGE_RISE, true, &ps2_keycode_callback);

	/* GPIO inputs (8 bits) */
	gpio_init(27);
	gpio_init(26);
	gpio_init(22);
	gpio_init(21);
	gpio_init(20);
	gpio_init(19);
	gpio_init(18);
	gpio_init(17);
	gpio_set_dir(27, GPIO_IN);
	gpio_set_dir(26, GPIO_IN);
	gpio_set_dir(22, GPIO_IN);
	gpio_set_dir(21, GPIO_IN);
	gpio_set_dir(20, GPIO_IN);
	gpio_set_dir(19, GPIO_IN);
	gpio_set_dir(18, GPIO_IN);
	gpio_set_dir(17, GPIO_IN);
	gpio_pull_up(27);
	gpio_pull_up(26);
	gpio_pull_up(22);
	gpio_pull_up(21);
	gpio_pull_up(20);
	gpio_pull_up(19);
	gpio_pull_up(18);
	gpio_pull_up(17);
}


void
terminal_loop(void)
{
	char c;
	char last_keycode;

	for (;;) {
		/* handle keyboard inputs */

		while (keycodes_ringbuf_get(&last_keycode) == 0) {
			handle_keycode(last_keycode);
		}

		/* get next char from UART (blocking) */
		/* TODO: setup timeout and blink cursor on timeout */
		int timeout;
		for (timeout = 5; timeout > 0; --timeout) {
			if (uart_is_readable(uart0)) {
				break;
			}
			sleep_ms(10);
		}
		if (timeout <= 0) {
			continue;
		}
		c  = uart_getc(uart0);

		if ((c >= 32) && (c <= 126)) {                       // Output printable characters
			/* char is printable */
			print_char(c);
			fs();
		}
		else {
			/* control characters */
			print_char(' ');

			switch(c) {
			case 0x08:
				/* backspace */
				bs();
				break;
			case 0x7F:
				/* behaves like backspace */
				bs();
				break;
			case 0x0D:
				/* carriage return */
				lf();
				break;
			case 0x03: /* fallthrough */
				/* Ctrl+C */
			case 0x1B: /* fallthrough */
				/* Esc */
			default:
				/* ignore */
				break;
			}
		}
	}
}

/* private functions definitions */

static void
fs(void)
{
	posx += incx;
	if (posx + charw >= width) {
		lf();
	}
}


static void
bs(void)
{
	posx -= incx;
	if (posx < startx) {
		posx = startx;
	}
}


static void
lf(void)
{
	posx = startx;
	posy += incy;
	if (posy + incy >= height) {
		posy -= incy;
		scroll_up(incy);
	}
}


static void
scroll_up(int rows)
{
	memcpy(bitmap, &bitmap[width * rows], (height - rows) * width);
	memset(&bitmap[width * (height - rows)], BLACK, rows * width);
}


static void
print_char(char c)
{
	//printf("print char: %d=%d\r\n",
	//         c, c - ' ');

	/* bitmap[height=384][width=512] */
	for (int i = posy; i < posy + charh; ++i) {
		memcpy(&bitmap[(i * width) + posx], font[c - ' '][i - posy], charw);
	}
	//if (c == ' ') {
	//	for (int i = posy; i < posy + charh; ++i) {
	//		memset(&bitmap[(i * width) + posx], BLACK, charw);
	//	}
	//} else {
	//	for (int i = posy; i < posy + charh; ++i) {
	//		memset(&bitmap[(i * width) + posx], WHITE, charw);
	//	}
	//}
}

static void
ps2_keycode_callback(uint gpio, uint32_t events)
{
	char keycode = 0;

	keycode |= (!!gpio_get(27)) << 0;
	keycode |= (!!gpio_get(26)) << 1;
	keycode |= (!!gpio_get(22)) << 2;
	keycode |= (!!gpio_get(21)) << 3;
	keycode |= (!!gpio_get(20)) << 4;
	keycode |= (!!gpio_get(19)) << 5;
	keycode |= (!!gpio_get(18)) << 6;
	keycode |= (!!gpio_get(17)) << 7;

	keycodes_ringbuf_put(keycode);
}


static int
keycodes_ringbuf_put(char kc)
{
	int next_iwrite = keycodes_ringbuf_inc(keycodes_ringbuf.iwrite);

	/* test if full */
	if (next_iwrite == keycodes_ringbuf.iread) {
		/* ringbuf full, do nothing */
		return -1;
	} else {
		/* write to buf, then increment iwrite */
		keycodes_ringbuf.buf[keycodes_ringbuf.iwrite] = kc;
		keycodes_ringbuf.iwrite = next_iwrite;
		return 0;
	}
}

static int
keycodes_ringbuf_get(char *kc)
{
	/* test if empty */
	if (keycodes_ringbuf.iread == keycodes_ringbuf.iwrite) {
		/* nothing to read */
		return -1;
	} else {
		*kc = keycodes_ringbuf.buf[keycodes_ringbuf.iread];
		keycodes_ringbuf.iread = keycodes_ringbuf_inc(keycodes_ringbuf.iread);
		return 0;
	}
}


static int
keycodes_ringbuf_inc(int idx)
{
	if (idx + 1 >= KEYCODES_MAX_NUM) {
		return 0;
	} else {
		return idx + 1;
	}
}


static int
keycodes_ringbuf_dec(int idx)
{
	if (idx - 1 < 0) {
		return KEYCODES_MAX_NUM;
	} else {
		return idx - 1;
	}
}


static void
handle_keycode(char kc)
{
	static int release_next = 0;
	static int ctrl_hold = 0;
	static int shift_hold = 0;

	printf("keycode 0x%x\n\r", kc);

	if (kc == 0xf0) {
		/* release next key */
		release_next = 1;
		return;
	}

	if (KC_IS_SHIFT(kc)) {
		shift_hold = 1;
	}

	if (release_next) {
		release_next = 0;
		if (KC_IS_SHIFT(kc)) {
			shift_hold = 0;
		}
		return;
	}

	if ((int) kc < 127) {
		char c = kbd_US[kc];
		if (shift_hold && ((c >= 'a') && (c <= 'z'))) {
			/* convert to uppercase */
			c -= 32;
		}
		printf("%c\n\r", c);
	}
}
