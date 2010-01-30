#include "emu.h"
#include "machine/teleprinter.h"
#include "machine/terminal.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/
#define TELEPRINTER_WIDTH 80
#define TELEPRINTER_HEIGHT 50
typedef struct _teleprinter_state teleprinter_state;
struct _teleprinter_state
{
	UINT8 buffer[TELEPRINTER_WIDTH*TELEPRINTER_HEIGHT];
	UINT8 x_pos;
	UINT8 last_code;
	UINT8 scan_line;
	devcb_resolved_write8 teleprinter_keyboard_func;
};

/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/
INLINE teleprinter_state *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == GENERIC_TELEPRINTER);

	return (teleprinter_state *)device->token;
}

INLINE const teleprinter_interface *get_interface(running_device *device)
{
	assert(device != NULL);
	assert(device->type == GENERIC_TELEPRINTER);
	return (const teleprinter_interface *) device->baseconfig().static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

static const UINT8 teleprinter_font[128*8] =
{
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x07,0x07,0x07,0x07,0x00,0x00,0x00,0x00,
  0x38,0x38,0x38,0x38,0x00,0x00,0x00,0x00,
  0x3f,0x3f,0x3f,0x3f,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x07,0x07,0x07,0x07,
  0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,
  0x38,0x38,0x38,0x38,0x07,0x07,0x07,0x07,
  0x3f,0x3f,0x3f,0x3f,0x07,0x07,0x07,0x07,
  0x00,0x00,0x00,0x00,0x38,0x38,0x38,0x38,
  0x07,0x07,0x07,0x07,0x38,0x38,0x38,0x38,
  0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,
  0x3f,0x3f,0x3f,0x3f,0x38,0x38,0x38,0x38,
  0x00,0x00,0x00,0x00,0x3f,0x3f,0x3f,0x3f,
  0x07,0x07,0x07,0x07,0x3f,0x3f,0x3f,0x3f,
  0x38,0x38,0x38,0x38,0x3f,0x3f,0x3f,0x3f,
  0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,
  0x00,0x00,0x00,0x00,0x2a,0x15,0x2a,0x15,
  0x2a,0x15,0x2a,0x15,0x00,0x00,0x00,0x00,
  0x3f,0x3f,0x3f,0x3f,0x2a,0x15,0x2a,0x15,
  0x2a,0x15,0x2a,0x15,0x3f,0x3f,0x3f,0x3f,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x08,0x08,0x08,0x08,0x08,0x00,0x08,0x00,
  0x14,0x14,0x14,0x00,0x00,0x00,0x00,0x00,
  0x14,0x14,0x3e,0x14,0x3e,0x14,0x14,0x00,
  0x08,0x3c,0x0a,0x1c,0x28,0x1e,0x08,0x00,
  0x06,0x26,0x10,0x08,0x04,0x32,0x30,0x00,
  0x08,0x14,0x14,0x0c,0x2a,0x12,0x2c,0x00,
  0x08,0x08,0x04,0x00,0x00,0x00,0x00,0x00,
  0x10,0x08,0x04,0x04,0x04,0x08,0x10,0x00,
  0x04,0x08,0x10,0x10,0x10,0x08,0x04,0x00,
  0x00,0x08,0x2a,0x1c,0x2a,0x08,0x00,0x00,
  0x00,0x08,0x08,0x3e,0x08,0x08,0x00,0x00,
  0x00,0x00,0x00,0x00,0x08,0x08,0x04,0x00,
  0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,
  0x00,0x20,0x10,0x08,0x04,0x02,0x00,0x00,
  0x1c,0x22,0x32,0x2a,0x26,0x22,0x1c,0x00,
  0x08,0x0c,0x08,0x08,0x08,0x08,0x1c,0x00,
  0x1c,0x22,0x20,0x18,0x04,0x02,0x3e,0x00,
  0x3e,0x20,0x10,0x18,0x20,0x22,0x1c,0x00,
  0x10,0x18,0x14,0x12,0x3e,0x10,0x10,0x00,
  0x3e,0x02,0x1e,0x20,0x20,0x22,0x1c,0x00,
  0x10,0x08,0x04,0x1c,0x22,0x22,0x1c,0x00,
  0x3e,0x20,0x10,0x08,0x04,0x04,0x04,0x00,
  0x1c,0x22,0x22,0x1c,0x22,0x22,0x1c,0x00,
  0x1c,0x22,0x22,0x1c,0x10,0x08,0x04,0x00,
  0x00,0x00,0x00,0x08,0x00,0x08,0x00,0x00,
  0x00,0x00,0x08,0x00,0x08,0x08,0x04,0x00,
  0x10,0x08,0x04,0x02,0x04,0x08,0x10,0x00,
  0x00,0x00,0x3e,0x00,0x3e,0x00,0x00,0x00,
  0x04,0x08,0x10,0x20,0x10,0x08,0x04,0x00,
  0x1c,0x22,0x20,0x10,0x08,0x00,0x08,0x00,
  0x1c,0x22,0x32,0x2a,0x3a,0x02,0x3c,0x00,
  0x08,0x14,0x22,0x22,0x3e,0x22,0x22,0x00,
  0x1e,0x22,0x22,0x1e,0x22,0x22,0x1e,0x00,
  0x1c,0x22,0x02,0x02,0x02,0x22,0x1c,0x00,
  0x1e,0x24,0x24,0x24,0x24,0x24,0x1e,0x00,
  0x3e,0x02,0x02,0x1e,0x02,0x02,0x3e,0x00,
  0x3e,0x02,0x02,0x1e,0x02,0x02,0x02,0x00,
  0x1c,0x22,0x02,0x02,0x32,0x22,0x3c,0x00,
  0x22,0x22,0x22,0x3e,0x22,0x22,0x22,0x00,
  0x1c,0x08,0x08,0x08,0x08,0x08,0x1c,0x00,
  0x38,0x10,0x10,0x10,0x10,0x12,0x0c,0x00,
  0x22,0x12,0x0a,0x06,0x0a,0x12,0x22,0x00,
  0x02,0x02,0x02,0x02,0x02,0x02,0x3e,0x00,
  0x22,0x36,0x2a,0x2a,0x22,0x22,0x22,0x00,
  0x22,0x22,0x26,0x2a,0x32,0x22,0x22,0x00,
  0x1c,0x22,0x22,0x22,0x22,0x22,0x1c,0x00,
  0x1e,0x22,0x22,0x1e,0x02,0x02,0x02,0x00,
  0x1c,0x22,0x22,0x22,0x2a,0x12,0x2c,0x00,
  0x1e,0x22,0x22,0x1e,0x0a,0x12,0x22,0x00,
  0x1c,0x22,0x02,0x1c,0x20,0x22,0x1c,0x00,
  0x3e,0x08,0x08,0x08,0x08,0x08,0x08,0x00,
  0x22,0x22,0x22,0x22,0x22,0x22,0x1c,0x00,
  0x22,0x22,0x22,0x14,0x14,0x08,0x08,0x00,
  0x22,0x22,0x22,0x2a,0x2a,0x2a,0x14,0x00,
  0x22,0x22,0x14,0x08,0x14,0x22,0x22,0x00,
  0x22,0x22,0x22,0x14,0x08,0x08,0x08,0x00,
  0x3e,0x20,0x10,0x08,0x04,0x02,0x3e,0x00,
  0x0e,0x02,0x02,0x02,0x02,0x02,0x0e,0x00,
  0x00,0x02,0x04,0x08,0x10,0x20,0x00,0x00,
  0x38,0x20,0x20,0x20,0x20,0x20,0x38,0x00,
  0x08,0x1c,0x2a,0x08,0x08,0x08,0x08,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,
  0x04,0x08,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x1c,0x20,0x3c,0x22,0x3c,0x00,
  0x02,0x02,0x1e,0x22,0x22,0x22,0x1e,0x00,
  0x00,0x00,0x3c,0x02,0x02,0x02,0x3c,0x00,
  0x20,0x20,0x3c,0x22,0x22,0x22,0x3c,0x00,
  0x00,0x00,0x1c,0x22,0x3e,0x02,0x1c,0x00,
  0x18,0x04,0x0e,0x04,0x04,0x04,0x04,0x00,
  0x00,0x00,0x3c,0x22,0x22,0x3c,0x20,0x18,
  0x02,0x02,0x1e,0x22,0x22,0x22,0x22,0x00,
  0x08,0x00,0x0c,0x08,0x08,0x08,0x1c,0x00,
  0x10,0x00,0x18,0x10,0x10,0x10,0x12,0x0c,
  0x02,0x02,0x22,0x12,0x0e,0x16,0x22,0x00,
  0x08,0x08,0x08,0x08,0x08,0x08,0x10,0x00,
  0x00,0x00,0x16,0x2a,0x2a,0x2a,0x2a,0x00,
  0x00,0x00,0x1a,0x26,0x22,0x22,0x22,0x00,
  0x00,0x00,0x1c,0x22,0x22,0x22,0x1c,0x00,
  0x00,0x00,0x1e,0x22,0x22,0x1e,0x02,0x02,
  0x00,0x00,0x3c,0x22,0x22,0x3c,0x20,0x20,
  0x00,0x00,0x34,0x0c,0x04,0x04,0x04,0x00,
  0x00,0x00,0x3c,0x02,0x1c,0x20,0x1e,0x00,
  0x08,0x08,0x1c,0x08,0x08,0x08,0x10,0x00,
  0x00,0x00,0x22,0x22,0x22,0x32,0x2c,0x00,
  0x00,0x00,0x22,0x22,0x22,0x14,0x08,0x00,
  0x00,0x00,0x22,0x22,0x2a,0x2a,0x14,0x00,
  0x00,0x00,0x22,0x14,0x08,0x14,0x22,0x00,
  0x00,0x00,0x22,0x22,0x14,0x08,0x04,0x02,
  0x00,0x00,0x3e,0x10,0x08,0x04,0x3e,0x00,
  0x10,0x08,0x08,0x04,0x08,0x08,0x10,0x00,
  0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x00,
  0x04,0x08,0x08,0x10,0x08,0x08,0x04,0x00,
  0x3e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x2a,0x15,0x2a,0x15,0x2a,0x15,0x2a,0x15
};

static void teleprinter_scroll_line(running_device *device)
{
	teleprinter_state *term = get_safe_token(device);

	memcpy(term->buffer,term->buffer+TELEPRINTER_WIDTH,(TELEPRINTER_HEIGHT-1)*TELEPRINTER_WIDTH);
	memset(term->buffer + TELEPRINTER_WIDTH*(TELEPRINTER_HEIGHT-1),0,TELEPRINTER_WIDTH);
}

static void teleprinter_write_char(running_device *device,UINT8 data) {
	teleprinter_state *term = get_safe_token(device);

	term->buffer[(TELEPRINTER_HEIGHT-1)*TELEPRINTER_WIDTH+term->x_pos] = data;
	term->x_pos++;
	if (term->x_pos > TELEPRINTER_WIDTH) {
		term->x_pos = 0;
		teleprinter_scroll_line(device);
	}
}

static void teleprinter_clear(running_device *device) {
	teleprinter_state *term = get_safe_token(device);

	memset(term->buffer,0,TELEPRINTER_WIDTH*TELEPRINTER_HEIGHT);
	term->x_pos = 0;
}

WRITE8_DEVICE_HANDLER ( teleprinter_write )
{
	teleprinter_state *term = get_safe_token(device);
	switch(data) {
		case 10: term->x_pos = 0;
				 teleprinter_scroll_line(device);
				 break;
		case 13: term->x_pos = 0; break;
		case  9: do {
					term->x_pos ++;
				 } while ((term->x_pos % 8)!=0);
				 break;
		case 16: break;
		default: teleprinter_write_char(device,data); break;
	}
}

/***************************************************************************
    VIDEO HARDWARE
***************************************************************************/
void generic_teleprinter_update(running_device *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	UINT8 code;
	int y, c, x, b;
	teleprinter_state *term = get_safe_token(device);

	for (y = 0; y < TELEPRINTER_HEIGHT; y++)
	{
		for (c = 0; c < 8; c++)
		{
			int horpos = 0;
			for (x = 0; x < TELEPRINTER_WIDTH; x++)
			{
				code = teleprinter_font[(term->buffer[y*TELEPRINTER_WIDTH + x]  & 0x7f) *8 + c];
				for (b = 0; b < 8; b++)
				{
					*BITMAP_ADDR16(bitmap, y*8 + c, horpos++) =  (code >> b) & 0x01 ? 0 : 1;
				}
			}
		}
	}
}

static TIMER_CALLBACK(keyboard_callback)
{
	teleprinter_state *term = get_safe_token((running_device *)ptr);
	term->last_code = terminal_keyboard_handler(machine, &term->teleprinter_keyboard_func, term->last_code, &term->scan_line);
}

/***************************************************************************
    VIDEO HARDWARE
***************************************************************************/
static VIDEO_START( teleprinter )
{

}

static VIDEO_UPDATE(teleprinter )
{
	running_device *devconf = devtag_get_device(screen->machine, TELEPRINTER_TAG);
	generic_teleprinter_update( devconf, bitmap, cliprect);
	return 0;
}

MACHINE_DRIVER_START( generic_teleprinter )
	MDRV_SCREEN_ADD(TELEPRINTER_SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(TELEPRINTER_WIDTH*8, TELEPRINTER_HEIGHT*8)
	MDRV_SCREEN_VISIBLE_AREA(0, TELEPRINTER_WIDTH*8-1, 0, TELEPRINTER_HEIGHT*8-1)
	MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(teleprinter)
	MDRV_VIDEO_UPDATE(teleprinter)
MACHINE_DRIVER_END


/*-------------------------------------------------
    DEVICE_START( teleprinter )
-------------------------------------------------*/

static DEVICE_START( teleprinter )
{
	teleprinter_state *term = get_safe_token(device);
	const teleprinter_interface *intf = get_interface(device);

	devcb_resolve_write8(&term->teleprinter_keyboard_func, &intf->teleprinter_keyboard_func, device);

	timer_pulse(device->machine, ATTOTIME_IN_HZ(240), (void*)device, 0, keyboard_callback);
}

/*-------------------------------------------------
    DEVICE_RESET( teleprinter )
-------------------------------------------------*/

static DEVICE_RESET( teleprinter )
{
	teleprinter_state *term = get_safe_token(device);

	teleprinter_clear(device);
	term->last_code = 0;
	term->scan_line = 0;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( teleprinter )
-------------------------------------------------*/

DEVICE_GET_INFO( teleprinter )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;												break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(teleprinter_state);							break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(teleprinter);					break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(teleprinter);					break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Generic Teleprinter");						break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Teleprinter");								break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

INPUT_PORTS_START( generic_teleprinter )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END
