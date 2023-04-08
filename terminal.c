#include "terminal.h"

#include "common.h"
#include "font.h"

#include "pico/stdlib.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/uart.h"

#include <stdio.h>
#include <string.h>


#define DEBUG_BUF_LEN 256


/* need those from cvideo.c */
//extern const int WHITE;
//extern const int BLACK;

/* private variables declarations */
/* position in pixel array */
static int posx = gapx;
static int posy = gapy;

/* debug string buffer */
static char debugbuf[DEBUG_BUF_LEN];


/* private functions declarations */
static void fs(void);
static void bs(void);
static void lf(void);
static void scroll_up(int rows);
static void print_char(char c);


/* public functions definitions */

void
terminal_init_uart(void)
{
	uart_init(uart0, 115200);
	gpio_set_function(12, GPIO_FUNC_UART);  // TX
	gpio_set_function(13, GPIO_FUNC_UART);  // RX
}


void
terminal_loop(void)
{
	char c;

	for (;;) {
		/* get next char from UART (blocking) */
		/* TODO: setup timeout and blink cursor on timeout */
		c  = uart_getc(uart0);

		/* DEBUGGING */
		snprintf(debugbuf, DEBUG_BUF_LEN, "rx %c %x, pos=%d,%d\r\n", c, c, posx, posy);
		uart_puts(uart0, debugbuf);

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
	snprintf(debugbuf, DEBUG_BUF_LEN, "print char: %d=%d\r\n",
	         c, c - ' ');
	uart_puts(uart0, debugbuf);

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
