#include "include/keyboard.h"
#include "types.h"
#include "assembly.h"

typedef struct
{
	BOOL caps;
	BOOL num;
	BOOL scroll;

	BOOL alt;
	BOOL shift;
	BOOL ctrl;

	BOOL extended;

	uint8_t scancode;
	keyboard_event_callback_t callback;
} g_kybrd_context;

g_kybrd_context context = {0};

uint32_t _kkybrd_scancode_std[] = {

	//! key			scancode
	KEY_UNKNOWN,	  // 0
	KEY_ESCAPE,		  // 1
	KEY_1,			  // 2
	KEY_2,			  // 3
	KEY_3,			  // 4
	KEY_4,			  // 5
	KEY_5,			  // 6
	KEY_6,			  // 7
	KEY_7,			  // 8
	KEY_8,			  // 9
	KEY_9,			  // 0xa
	KEY_0,			  // 0xb
	KEY_MINUS,		  // 0xc
	KEY_EQUAL,		  // 0xd
	KEY_BACKSPACE,	  // 0xe
	KEY_TAB,		  // 0xf
	KEY_Q,			  // 0x10
	KEY_W,			  // 0x11
	KEY_E,			  // 0x12
	KEY_R,			  // 0x13
	KEY_T,			  // 0x14
	KEY_Y,			  // 0x15
	KEY_U,			  // 0x16
	KEY_I,			  // 0x17
	KEY_O,			  // 0x18
	KEY_P,			  // 0x19
	KEY_LEFTBRACKET,  // 0x1a
	KEY_RIGHTBRACKET, // 0x1b
	KEY_RETURN,		  // 0x1c
	KEY_LCTRL,		  // 0x1d
	KEY_A,			  // 0x1e
	KEY_S,			  // 0x1f
	KEY_D,			  // 0x20
	KEY_F,			  // 0x21
	KEY_G,			  // 0x22
	KEY_H,			  // 0x23
	KEY_J,			  // 0x24
	KEY_K,			  // 0x25
	KEY_L,			  // 0x26
	KEY_SEMICOLON,	  // 0x27
	KEY_QUOTE,		  // 0x28
	KEY_GRAVE,		  // 0x29
	KEY_LSHIFT,		  // 0x2a
	KEY_BACKSLASH,	  // 0x2b
	KEY_Z,			  // 0x2c
	KEY_X,			  // 0x2d
	KEY_C,			  // 0x2e
	KEY_V,			  // 0x2f
	KEY_B,			  // 0x30
	KEY_N,			  // 0x31
	KEY_M,			  // 0x32
	KEY_COMMA,		  // 0x33
	KEY_DOT,		  // 0x34
	KEY_SLASH,		  // 0x35
	KEY_RSHIFT,		  // 0x36
	KEY_KP_ASTERISK,  // 0x37
	KEY_RALT,		  // 0x38
	KEY_SPACE,		  // 0x39
	KEY_CAPSLOCK,	  // 0x3a
	KEY_F1,			  // 0x3b
	KEY_F2,			  // 0x3c
	KEY_F3,			  // 0x3d
	KEY_F4,			  // 0x3e
	KEY_F5,			  // 0x3f
	KEY_F6,			  // 0x40
	KEY_F7,			  // 0x41
	KEY_F8,			  // 0x42
	KEY_F9,			  // 0x43
	KEY_F10,		  // 0x44
	KEY_KP_NUMLOCK,	  // 0x45
	KEY_SCROLLLOCK,	  // 0x46
	KEY_HOME,		  // 0x47
	KEY_KP_8,		  // 0x48	//keypad up arrow
	KEY_PAGEUP,		  // 0x49
	KEY_KP_2,		  // 0x50	//keypad down arrow
	KEY_KP_3,		  // 0x51	//keypad page down
	KEY_KP_0,		  // 0x52	//keypad insert key
	KEY_KP_DECIMAL,	  // 0x53	//keypad delete key
	KEY_UNKNOWN,	  // 0x54
	KEY_UNKNOWN,	  // 0x55
	KEY_UNKNOWN,	  // 0x56
	KEY_F11,		  // 0x57
	KEY_F12			  // 0x58
};

uint8_t kybrd_ctrl_read_status()
{
	return port_inb(KYBRD_CONTROLLER_PORT);
}

void kybrd_wait_for_controller()
{
	for (;;)
	{
		if ((kybrd_ctrl_read_status() & KYBRD_CTRL_STATS_MASK_IN_BUF) == EMPTY_BUFFER)
		{
			break;
		}
	}
}

void kybrd_ctrl_send_cmd(uint8_t cmd)
{
	kybrd_wait_for_controller();

	port_outb(KYBRD_CONTROLLER_PORT, cmd);
}

uint8_t kybrd_enc_read_buf()
{
	return port_inb(KYBRD_ENCODER_PORT);
}

void kybrd_enc_send_cmd(uint8_t cmd)
{
	kybrd_wait_for_controller();

	port_outb(KYBRD_ENCODER_PORT, cmd);
}

void kybrd_set_leds(BOOL num, BOOL scroll, BOOL caps)
{
	uint8_t data = 0;

	if (scroll)
	{
		data |= 1;
	}

	if (num)
	{
		data |= 2;
	}

	if (caps)
	{
		data |= 4;
	}

	kybrd_enc_send_cmd(KYBRD_ENC_CMD_SET_LED);
	kybrd_enc_send_cmd(data);
}

void kybrd_disable()
{
	kybrd_ctrl_send_cmd(0xAD);
}

void kybrd_enable()
{
	kybrd_ctrl_send_cmd(0xAE);
}

void kybrd_reset_system()
{
	kybrd_ctrl_send_cmd(KYBRD_CONTROLLER_PORT);
	kybrd_enc_send_cmd(0xfe);
}

void kybrd_init()
{
	kybrd_set_leds(FALSE, FALSE, FALSE);
}

void kybrd_irq_handler()
{
	uint8_t code;

	if (!(kybrd_ctrl_read_status() & KYBRD_CTRL_STATS_MASK_OUT_BUF))
	{
		return;
	}

	code = kybrd_enc_read_buf();

	if (code == 0xE0 || code == 0xE1)
	{
		context.extended = TRUE;
		return;
	}
	else
	{
		context.extended = FALSE;
	}

	// Test if break code
	if (code & 0x80)
	{
		code -= 0x80;

		uint32_t key = _kkybrd_scancode_std[code];

		switch (key)
		{
		case KEY_LCTRL:
		case KEY_RCTRL:
			context.ctrl = FALSE;
			break;

		case KEY_LSHIFT:
		case KEY_RSHIFT:
			context.shift = FALSE;
			break;

		case KEY_LALT:
		case KEY_RALT:
			context.alt = FALSE;
			break;

		default:
			break; // ???
		}

		context.callback(context.scancode, FALSE);
	}
	else
	{
		context.scancode = code;

		uint32_t key = _kkybrd_scancode_std[code];

		switch (key)
		{
		case KEY_LCTRL:
		case KEY_RCTRL:
			context.ctrl = TRUE;
			break;

		case KEY_LSHIFT:
		case KEY_RSHIFT:
			context.shift = TRUE;
			break;

		case KEY_LALT:
		case KEY_RALT:
			context.alt = TRUE;
			break;

		case KEY_CAPSLOCK:
			context.caps = (context.caps) ? FALSE : TRUE;
			kybrd_set_leds(context.num, context.scroll, context.caps);
			break;

		case KEY_KP_NUMLOCK:
			context.num = (context.num) ? FALSE : TRUE;
			kybrd_set_leds(context.num, context.scroll, context.caps);
			break;

		case KEY_SCROLLLOCK:
			context.scroll = (context.scroll) ? FALSE : TRUE;
			kybrd_set_leds(context.num, context.scroll, context.caps);
			break;

		default:
			break;
		}

		context.callback(context.scancode, TRUE);
	}

	/*
	switch (code)
	{
		case KYBRD_ERR_BAT_FAILED:
		case KYBRD_ERR_DIAG_FAILED:
		case KYBRD_ERR_RESEND_CMD:
	}
	*/
}

uint8_t get_last_scancode()
{
	return context.scancode;
}

uint32_t kybrd_key_to_ascii(uint8_t code)
{
	uint32_t key = _kkybrd_scancode_std[code];

	if (isascii(key))
	{

		if (context.shift || context.caps)
			if (key >= 'a' && key <= 'z')
			{
				key -= 32;
			}

		if (context.shift && !context.caps)
		{
			if (key >= '0' && key <= '9')
			{
				switch (key)
				{

				case '0':
					key = KEY_RIGHTPARENTHESIS;
					break;
				case '1':
					key = KEY_EXCLAMATION;
					break;
				case '2':
					key = KEY_AT;
					break;
				case '3':
					key = KEY_HASH;
					break;
				case '4':
					key = KEY_DOLLAR;
					break;
				case '5':
					key = KEY_PERCENT;
					break;
				case '6':
					key = KEY_CARRET;
					break;
				case '7':
					key = KEY_AMPERSAND;
					break;
				case '8':
					key = KEY_ASTERISK;
					break;
				case '9':
					key = KEY_LEFTPARENTHESIS;
					break;
				}
			}
			else
			{

				switch (key)
				{
				case KEY_COMMA:
					key = KEY_LESS;
					break;

				case KEY_DOT:
					key = KEY_GREATER;
					break;

				case KEY_SLASH:
					key = KEY_QUESTION;
					break;

				case KEY_SEMICOLON:
					key = KEY_COLON;
					break;

				case KEY_QUOTE:
					key = KEY_QUOTEDOUBLE;
					break;

				case KEY_LEFTBRACKET:
					key = KEY_LEFTCURL;
					break;

				case KEY_RIGHTBRACKET:
					key = KEY_RIGHTCURL;
					break;

				case KEY_GRAVE:
					key = KEY_TILDE;
					break;

				case KEY_MINUS:
					key = KEY_UNDERSCORE;
					break;

				case KEY_PLUS:
					key = KEY_EQUAL;
					break;

				case KEY_BACKSLASH:
					key = KEY_BAR;
					break;
				}
			}
		}
		return key;
	}

	return 0;
}

void kybrd_set_event_callback(keyboard_event_callback_t callback)
{
	context.callback = callback;
}
