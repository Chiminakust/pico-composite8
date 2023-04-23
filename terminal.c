#include "terminal.h"

#include "common.h"
#include "font.h"

#include "bsp/board.h"
#include "tusb.h"

#include "pico/stdlib.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/uart.h"

#include <stdio.h>
#include <string.h>


#define DEBUG_BUF_LEN 256

// not sure what that does
//#define USE_ANSI_ESCAPE 0

#define MAX_REPORT 4

static uint8_t const keycode2ascii[128][2] = { HID_KEYCODE_TO_ASCII };

static struct {
	uint8_t report_count;
	tuh_hid_report_info_t report_info[MAX_REPORT];
} hid_info[CFG_TUH_HID];

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
static void process_kbd_report(hid_keyboard_report_t const *report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance,
                                    uint8_t const *report, uint16_t len);
void hid_app_task(void);


/* public functions definitions */

void
terminal_init_uart(void)
{
	//board_init();

	/* init tiny usb */
	//tusb_init();
	//tuh_init(BOARD_TUH_RHPORT);

	uart_init(uart0, 115200);
	gpio_set_function(12, GPIO_FUNC_UART);  // TX
	gpio_set_function(13, GPIO_FUNC_UART);  // RX

	//printf("terminal init done\r\n");
}


void
terminal_loop(void)
{
	char c;

	for (;;) {
		/* usb host task */
		//tuh_task();
		/* usb hid (keyboard) task */
		//hid_app_task(); // does nothing anyway
		//printf("loop\r\n");

		/* get next char from UART (blocking) */
		/* TODO: setup timeout and blink cursor on timeout */
		int timeout;
		for (timeout = 5; timeout > 0; --timeout) {
			if (uart_is_readable(uart0)) {
				break;
			}
			sleep_ms(100);
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

void
hid_app_task(void)
{
	/* nothing to do */
}


/* tiny usb callbacks */

/* usb HID device mounted callback */
void
tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                 uint8_t const *desc_report, uint16_t desc_len)
{
	//printf("hid mount callback\r\n");
	/* itf_protocol = 0: None, 1: Keyboard, 2: mouse */
	uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

	//printf("device mount addr %x instance %x\r\n", dev_addr, instance);

	/*
	 * By default, host stack will use activate boot protocol on supported interface.
	 * We only need to parse generic report descriptor with the built-in parser.
	 */
	if (itf_protocol == HID_ITF_PROTOCOL_NONE) {
		hid_info[instance].report_count = tuh_hid_parse_report_descriptor(
			hid_info[instance].report_info,
			MAX_REPORT,
			desc_report,
			desc_len
			);
	}

	if ( !tuh_hid_receive_report(dev_addr, instance) )
	{
		/* error */
	}
}

/* device unmounted callback */
void
tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
	/* nothing to do */
	//printf("unmount addr %x instance %x\r\n", dev_addr, instance);
}

/* called when received report from device via interrupt endpoint */
void
tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                           uint8_t const* report, uint16_t len)
{
	uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

	//printf("%x-%x report received\r\n", dev_addr, instance);

	switch (itf_protocol) {
	case HID_ITF_PROTOCOL_KEYBOARD:
		//printf("it's a keyboard\r\n", dev_addr, instance);

		process_kbd_report( (hid_keyboard_report_t const*) report );
		break;
	default:
		// Generic report requires matching ReportID and contents with previous parsed report info
		process_generic_report(dev_addr, instance, report, len);
		break;
	}

	// continue to request to receive report
	if ( !tuh_hid_receive_report(dev_addr, instance) )
	{
		//printf("error when requesting to continue receiving reports\r\n");
		/* error */
	}
}

/* usb keyboard stuff */

/* find new keys in previous keys */
static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode)
{
	for(uint8_t i = 0; i < 6; ++i) {
		if (report->keycode[i] == keycode) {
			return true;
		}
	}

	return false;
}

static void
process_kbd_report(hid_keyboard_report_t const *report)
{
	/* previous report to check key release */
	static hid_keyboard_report_t prev_report = { 0, 0, { 0 } };

	//printf("kb report process\r\n");

	/* ignore control (non-printable) key effects */
	for (uint8_t i = 0; i < 6; ++i) {
		if (report->keycode[i]) {

			//printf("i:%x ch:%x\r\n",
			//       i, keycode2ascii[report->keycode[i]][(report->modifier &
			//                                             (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT)) ? 1 : 0]);

			if (find_key_in_report(&prev_report, report->keycode[i])) {
				/* was in previous, ignore */
			} else {
				bool const is_shift = (report->modifier &
					(KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT));
				uint8_t ch = keycode2ascii[report->keycode[i]][is_shift ? 1 : 0];
				/* write to UART here */
				uart_putc(uart0, ch);
				/* LF for CR */
				if (ch == '\r') {
					uart_putc(uart0, '\n');
				}
			}
		}
	}

	prev_report = *report;
}

/* generic report */
static void
process_generic_report(uint8_t dev_addr, uint8_t instance,
                       uint8_t const* report, uint16_t len)
{
	(void) dev_addr;

	//printf("process generic report\r\n");

	uint8_t const rpt_count = hid_info[instance].report_count;
	tuh_hid_report_info_t *rpt_info_arr = hid_info[instance].report_info;
	tuh_hid_report_info_t *rpt_info = NULL;

	if (rpt_count == 1 && rpt_info_arr[0].report_id == 0) {
		/* simple report without report id as 1st byte */
		rpt_info = &rpt_info_arr[0];
	} else {
		/* composite report, 1st byte = report id, data starts from 2nd byte */
		uint8_t const rpt_id = report[0];

		/* find report id in the array */
		for (uint8_t i = 0; i < rpt_count; ++i) {
			if (rpt_id == rpt_info_arr[i].report_id) {
				rpt_info = &rpt_info_arr[i];
				break;
			}
		}

		report++;
		len--;
	}

	if (!rpt_info) {
		/* error */
		return;
	}

	/* determine next handler */
	if (rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP) {
		switch (rpt_info->usage) {
		case HID_USAGE_DESKTOP_KEYBOARD:
			process_kbd_report((hid_keyboard_report_t const *) report);
			break;
		default:
			break;
		}
	}
}
