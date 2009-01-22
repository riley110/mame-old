/***************************************************************************

  a7800.c

  Driver file to handle emulation of the Atari 7800.

  Dan Boris

    2002/05/13 kubecj   added more banks for bankswitching
                            added PAL machine description
                            changed clock to be precise

***************************************************************************/

#include "driver.h"
#include "deprecat.h"
#include "cpu/m6502/m6502.h"
#include "sound/pokey.h"
#include "sound/tiaintf.h"
#include "devices/cartslot.h"
#include "machine/6532riot.h"
#include "includes/a7800.h"


#define A7800_NTSC_Y1	XTAL_14_31818MHz
#define CLK_PAL 1773447


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( a7800_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x001f) AM_MIRROR(0x300) AM_READWRITE(a7800_TIA_r, a7800_TIA_w)
	AM_RANGE(0x0020, 0x003f) AM_MIRROR(0x300) AM_READWRITE(a7800_MARIA_r, a7800_MARIA_w)
	AM_RANGE(0x0040, 0x00ff) AM_READWRITE(SMH_BANK5, a7800_RAM0_w)	/* RAM (6116 block 0) */
	AM_RANGE(0x0140, 0x01ff) AM_RAMBANK(6)	/* RAM (6116 block 1) */
	AM_RANGE(0x0280, 0x02ff) AM_DEVREADWRITE(RIOT6532, "riot", riot6532_r, riot6532_w)
	AM_RANGE(0x0480, 0x04ff) AM_MIRROR(0x100) AM_RAM	/* RIOT RAM */
	AM_RANGE(0x1800, 0x27ff) AM_RAM
	AM_RANGE(0x2800, 0x2fff) AM_RAMBANK(7)	/* MAINRAM */
	AM_RANGE(0x3000, 0x37ff) AM_RAMBANK(7)	/* MAINRAM */
	AM_RANGE(0x3800, 0x3fff) AM_RAMBANK(7)	/* MAINRAM */
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK(1)						/* f18 hornet */
	AM_RANGE(0x8000, 0x9fff) AM_ROMBANK(2)						/* sc */
	AM_RANGE(0xa000, 0xbfff) AM_ROMBANK(3)						/* sc + ac */
	AM_RANGE(0xc000, 0xdfff) AM_ROMBANK(4)						/* ac */
	AM_RANGE(0xe000, 0xffff) AM_ROM
	AM_RANGE(0x4000, 0xffff) AM_WRITE(a7800_cart_w)
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( a7800 )
	PORT_START("joysticks")            /* IN0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1) PORT_8WAY

	PORT_START("buttons")              /* IN1 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1)       PORT_PLAYER(2)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON1)       PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON2)       PORT_PLAYER(2)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON2)       PORT_PLAYER(1)
	PORT_BIT(0xF0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("vblank")               /* IN2 */
	PORT_BIT(0x7F, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_VBLANK)

	PORT_START("console_buttons")      /* IN3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER)  PORT_NAME("Reset")         PORT_CODE(KEYCODE_R)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER)  PORT_NAME("Select")        PORT_CODE(KEYCODE_S)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER)  PORT_NAME(DEF_STR(Pause))  PORT_CODE(KEYCODE_O)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_DIPNAME(0x40, 0x00, "Left Difficulty Switch")
	PORT_DIPSETTING(0x40, "A" )
	PORT_DIPSETTING(0x00, "B" )
	PORT_DIPNAME(0x80, 0x00, "Right Difficulty Switch")
	PORT_DIPSETTING(0x80, "A" )
	PORT_DIPSETTING(0x00, "B" )
INPUT_PORTS_END


/***************************************************************************
    PALETTE
***************************************************************************/

#define NTSC_GREY    \
	MAKE_RGB(0x00,0x00,0x00), MAKE_RGB(0x25,0x25,0x25), MAKE_RGB(0x34,0x34,0x34), MAKE_RGB(0x4e,0x4e,0x4e),	\
	MAKE_RGB(0x68,0x68,0x68), MAKE_RGB(0x75,0x75,0x75), MAKE_RGB(0x8e,0x8e,0x8e), MAKE_RGB(0xa4,0xa4,0xa4),	\
	MAKE_RGB(0xb8,0xb8,0xb8), MAKE_RGB(0xc5,0xc5,0xc5), MAKE_RGB(0xd0,0xd0,0xd0), MAKE_RGB(0xd7,0xd7,0xd7),	\
	MAKE_RGB(0xe1,0xe1,0xe1), MAKE_RGB(0xea,0xea,0xea), MAKE_RGB(0xf4,0xf4,0xf4), MAKE_RGB(0xff,0xff,0xff)

#define NTSC_GOLD \
	MAKE_RGB(0x41,0x20,0x00), MAKE_RGB(0x54,0x28,0x00), MAKE_RGB(0x76,0x37,0x00), MAKE_RGB(0x9a,0x50,0x00), \
	MAKE_RGB(0xc3,0x68,0x06), MAKE_RGB(0xe4,0x7b,0x07), MAKE_RGB(0xff,0x91,0x1a), MAKE_RGB(0xff,0xab,0x1d), \
	MAKE_RGB(0xff,0xc5,0x1f), MAKE_RGB(0xff,0xd0,0x3b), MAKE_RGB(0xff,0xd8,0x4c), MAKE_RGB(0xff,0xe6,0x51), \
	MAKE_RGB(0xff,0xf4,0x56), MAKE_RGB(0xff,0xf9,0x70), MAKE_RGB(0xff,0xff,0x90), MAKE_RGB(0xff,0xff,0xaa)

#define NTSC_ORANGE \
	MAKE_RGB(0x45,0x19,0x04), MAKE_RGB(0x72,0x1e,0x11), MAKE_RGB(0x9f,0x24,0x1e), MAKE_RGB(0xb3,0x3a,0x20), \
	MAKE_RGB(0xc8,0x51,0x20), MAKE_RGB(0xe3,0x69,0x20), MAKE_RGB(0xfc,0x81,0x20), MAKE_RGB(0xfd,0x8c,0x25), \
	MAKE_RGB(0xfe,0x98,0x2c), MAKE_RGB(0xff,0xae,0x38), MAKE_RGB(0xff,0xb9,0x46), MAKE_RGB(0xff,0xbf,0x51), \
	MAKE_RGB(0xff,0xc6,0x6d), MAKE_RGB(0xff,0xd5,0x87), MAKE_RGB(0xff,0xe4,0x98), MAKE_RGB(0xff,0xe6,0xab)

#define NTSC_RED_ORANGE \
	MAKE_RGB(0x5d,0x1f,0x0c), MAKE_RGB(0x7a,0x24,0x0d), MAKE_RGB(0x98,0x2c,0x0e), MAKE_RGB(0xb0,0x2f,0x0f), \
	MAKE_RGB(0xbf,0x36,0x24), MAKE_RGB(0xd3,0x4e,0x2a), MAKE_RGB(0xe7,0x62,0x3e), MAKE_RGB(0xf3,0x6e,0x4a), \
	MAKE_RGB(0xfd,0x78,0x54), MAKE_RGB(0xff,0x8a,0x6a), MAKE_RGB(0xff,0x98,0x7c), MAKE_RGB(0xff,0xa4,0x8b), \
	MAKE_RGB(0xff,0xb3,0x9e), MAKE_RGB(0xff,0xc2,0xb2), MAKE_RGB(0xff,0xd0,0xc3), MAKE_RGB(0xff,0xda,0xd0)

#define NTSC_PINK \
	MAKE_RGB(0x4a,0x17,0x00), MAKE_RGB(0x72,0x1f,0x00), MAKE_RGB(0xa8,0x13,0x00), MAKE_RGB(0xc8,0x21,0x0a), \
	MAKE_RGB(0xdf,0x25,0x12), MAKE_RGB(0xec,0x3b,0x24), MAKE_RGB(0xfa,0x52,0x36), MAKE_RGB(0xfc,0x61,0x48), \
	MAKE_RGB(0xff,0x70,0x5f), MAKE_RGB(0xff,0x7e,0x7e), MAKE_RGB(0xff,0x8f,0x8f), MAKE_RGB(0xff,0x9d,0x9e), \
	MAKE_RGB(0xff,0xab,0xad), MAKE_RGB(0xff,0xb9,0xbd), MAKE_RGB(0xff,0xc7,0xce), MAKE_RGB(0xff,0xca,0xde)

#define NTSC_PURPLE \
	MAKE_RGB(0x49,0x00,0x36), MAKE_RGB(0x66,0x00,0x4b), MAKE_RGB(0x80,0x03,0x5f), MAKE_RGB(0x95,0x0f,0x74), \
	MAKE_RGB(0xaa,0x22,0x88), MAKE_RGB(0xba,0x3d,0x99), MAKE_RGB(0xca,0x4d,0xa9), MAKE_RGB(0xd7,0x5a,0xb6), \
	MAKE_RGB(0xe4,0x67,0xc3), MAKE_RGB(0xef,0x72,0xce), MAKE_RGB(0xfb,0x7e,0xda), MAKE_RGB(0xff,0x8d,0xe1), \
	MAKE_RGB(0xff,0x9d,0xe5), MAKE_RGB(0xff,0xa5,0xe7), MAKE_RGB(0xff,0xaf,0xea), MAKE_RGB(0xff,0xb8,0xec)

#define NTSC_PURPLE_BLUE \
	MAKE_RGB(0x48,0x03,0x6c), MAKE_RGB(0x5c,0x04,0x88), MAKE_RGB(0x65,0x0d,0x90), MAKE_RGB(0x7b,0x23,0xa7), \
	MAKE_RGB(0x93,0x3b,0xbf), MAKE_RGB(0x9d,0x45,0xc9), MAKE_RGB(0xa7,0x4f,0xd3), MAKE_RGB(0xb2,0x5a,0xde), \
	MAKE_RGB(0xbd,0x65,0xe9), MAKE_RGB(0xc5,0x6d,0xf1), MAKE_RGB(0xce,0x76,0xfa), MAKE_RGB(0xd5,0x83,0xff), \
	MAKE_RGB(0xda,0x90,0xff), MAKE_RGB(0xde,0x9c,0xff), MAKE_RGB(0xe2,0xa9,0xff), MAKE_RGB(0xe6,0xb6,0xff)

#define NTSC_BLUE1 \
	MAKE_RGB(0x05,0x1e,0x81), MAKE_RGB(0x06,0x26,0xa5), MAKE_RGB(0x08,0x2f,0xca), MAKE_RGB(0x26,0x3d,0xd4), \
	MAKE_RGB(0x44,0x4c,0xde), MAKE_RGB(0x4f,0x5a,0xec), MAKE_RGB(0x5a,0x68,0xff), MAKE_RGB(0x65,0x75,0xff), \
	MAKE_RGB(0x71,0x83,0xff), MAKE_RGB(0x80,0x91,0xff), MAKE_RGB(0x90,0xa0,0xff), MAKE_RGB(0x97,0xa9,0xff), \
	MAKE_RGB(0x9f,0xb2,0xff), MAKE_RGB(0xaf,0xbe,0xff), MAKE_RGB(0xc0,0xcb,0xff), MAKE_RGB(0xcd,0xd3,0xff)

#define NTSC_BLUE2 \
	MAKE_RGB(0x0b,0x07,0x79), MAKE_RGB(0x20,0x1c,0x8e), MAKE_RGB(0x35,0x31,0xa3), MAKE_RGB(0x46,0x42,0xb4), \
	MAKE_RGB(0x57,0x53,0xc5), MAKE_RGB(0x61,0x5d,0xcf), MAKE_RGB(0x6d,0x69,0xdb), MAKE_RGB(0x7b,0x77,0xe9), \
	MAKE_RGB(0x89,0x85,0xf7), MAKE_RGB(0x91,0x8d,0xff), MAKE_RGB(0x9c,0x98,0xff), MAKE_RGB(0xa7,0xa4,0xff), \
	MAKE_RGB(0xb2,0xaf,0xff), MAKE_RGB(0xbb,0xb8,0xff), MAKE_RGB(0xc3,0xc1,0xff), MAKE_RGB(0xd3,0xd1,0xff)

#define NTSC_LIGHT_BLUE \
	MAKE_RGB(0x1d,0x29,0x5a), MAKE_RGB(0x1d,0x38,0x76), MAKE_RGB(0x1d,0x48,0x92), MAKE_RGB(0x1d,0x5c,0xac), \
	MAKE_RGB(0x1d,0x71,0xc6), MAKE_RGB(0x32,0x86,0xcf), MAKE_RGB(0x48,0x9b,0xd9), MAKE_RGB(0x4e,0xa8,0xec), \
	MAKE_RGB(0x55,0xb6,0xff), MAKE_RGB(0x69,0xca,0xff), MAKE_RGB(0x74,0xcb,0xff), MAKE_RGB(0x82,0xd3,0xff), \
	MAKE_RGB(0x8d,0xda,0xff), MAKE_RGB(0x9f,0xd4,0xff), MAKE_RGB(0xb4,0xe2,0xff), MAKE_RGB(0xc0,0xeb,0xff)

#define NTSC_TURQUOISE \
	MAKE_RGB(0x00,0x4b,0x59), MAKE_RGB(0x00,0x5d,0x6e), MAKE_RGB(0x00,0x6f,0x84), MAKE_RGB(0x00,0x84,0x9c), \
	MAKE_RGB(0x00,0x99,0xbf), MAKE_RGB(0x00,0xab,0xca), MAKE_RGB(0x00,0xbc,0xde), MAKE_RGB(0x00,0xd0,0xf5), \
	MAKE_RGB(0x10,0xdc,0xff), MAKE_RGB(0x3e,0xe1,0xff), MAKE_RGB(0x64,0xe7,0xff), MAKE_RGB(0x76,0xea,0xff), \
	MAKE_RGB(0x8b,0xed,0xff), MAKE_RGB(0x9a,0xef,0xff), MAKE_RGB(0xb1,0xf3,0xff), MAKE_RGB(0xc7,0xf6,0xff)

#define NTSC_GREEN_BLUE	\
	MAKE_RGB(0x00,0x48,0x00), MAKE_RGB(0x00,0x54,0x00), MAKE_RGB(0x03,0x6b,0x03), MAKE_RGB(0x0e,0x76,0x0e), \
	MAKE_RGB(0x18,0x80,0x18), MAKE_RGB(0x27,0x92,0x27), MAKE_RGB(0x36,0xa4,0x36), MAKE_RGB(0x4e,0xb9,0x4e), \
	MAKE_RGB(0x51,0xcd,0x51), MAKE_RGB(0x72,0xda,0x72), MAKE_RGB(0x7c,0xe4,0x7c), MAKE_RGB(0x85,0xed,0x85), \
	MAKE_RGB(0x99,0xf2,0x99), MAKE_RGB(0xb3,0xf7,0xb3), MAKE_RGB(0xc3,0xf9,0xc3), MAKE_RGB(0xcd,0xfc,0xcd)

#define NTSC_GREEN \
	MAKE_RGB(0x16,0x40,0x00), MAKE_RGB(0x1c,0x53,0x00), MAKE_RGB(0x23,0x66,0x00), MAKE_RGB(0x28,0x78,0x00), \
	MAKE_RGB(0x2e,0x8c,0x00), MAKE_RGB(0x3a,0x98,0x0c), MAKE_RGB(0x47,0xa5,0x19), MAKE_RGB(0x51,0xaf,0x23), \
	MAKE_RGB(0x5c,0xba,0x2e), MAKE_RGB(0x71,0xcf,0x43), MAKE_RGB(0x85,0xe3,0x57), MAKE_RGB(0x8d,0xeb,0x5f), \
	MAKE_RGB(0x97,0xf5,0x69), MAKE_RGB(0xa0,0xfe,0x72), MAKE_RGB(0xb1,0xff,0x8a), MAKE_RGB(0xbc,0xff,0x9a)

#define NTSC_YELLOW_GREEN \
	MAKE_RGB(0x2c,0x35,0x00), MAKE_RGB(0x38,0x44,0x00), MAKE_RGB(0x44,0x52,0x00), MAKE_RGB(0x49,0x56,0x00), \
	MAKE_RGB(0x60,0x71,0x00), MAKE_RGB(0x6c,0x7f,0x00), MAKE_RGB(0x79,0x8d,0x0a), MAKE_RGB(0x8b,0x9f,0x1c), \
	MAKE_RGB(0x9e,0xb2,0x2f), MAKE_RGB(0xab,0xbf,0x3c), MAKE_RGB(0xb8,0xcc,0x49), MAKE_RGB(0xc2,0xd6,0x53), \
	MAKE_RGB(0xcd,0xe1,0x53), MAKE_RGB(0xdb,0xef,0x6c), MAKE_RGB(0xe8,0xfc,0x79), MAKE_RGB(0xf2,0xff,0xab)

#define NTSC_ORANGE_GREEN \
	MAKE_RGB(0x46,0x3a,0x09), MAKE_RGB(0x4d,0x3f,0x09), MAKE_RGB(0x54,0x45,0x09), MAKE_RGB(0x6c,0x58,0x09), \
	MAKE_RGB(0x90,0x76,0x09), MAKE_RGB(0xab,0x8b,0x0a), MAKE_RGB(0xc1,0xa1,0x20), MAKE_RGB(0xd0,0xb0,0x2f), \
	MAKE_RGB(0xde,0xbe,0x3d), MAKE_RGB(0xe6,0xc6,0x45), MAKE_RGB(0xed,0xcd,0x4c), MAKE_RGB(0xf5,0xd8,0x62), \
	MAKE_RGB(0xfb,0xe2,0x76), MAKE_RGB(0xfc,0xee,0x98), MAKE_RGB(0xfd,0xf3,0xa9), MAKE_RGB(0xfd,0xf3,0xbe)

#define NTSC_LIGHT_ORANGE \
	MAKE_RGB(0x40,0x1a,0x02), MAKE_RGB(0x58,0x1f,0x05), MAKE_RGB(0x70,0x24,0x08), MAKE_RGB(0x8d,0x3a,0x13), \
	MAKE_RGB(0xab,0x51,0x1f), MAKE_RGB(0xb5,0x64,0x27), MAKE_RGB(0xbf,0x77,0x30), MAKE_RGB(0xd0,0x85,0x3a), \
	MAKE_RGB(0xe1,0x93,0x44), MAKE_RGB(0xed,0xa0,0x4e), MAKE_RGB(0xf9,0xad,0x58), MAKE_RGB(0xfc,0xb7,0x5c), \
	MAKE_RGB(0xff,0xc1,0x60), MAKE_RGB(0xff,0xca,0x69), MAKE_RGB(0xff,0xcf,0x7e), MAKE_RGB(0xff,0xda,0x96)

static const rgb_t a7800_palette[256*3] =
{
	NTSC_GREY,
	NTSC_GOLD,
	NTSC_ORANGE,
	NTSC_RED_ORANGE,
	NTSC_PINK,
	NTSC_PURPLE,
	NTSC_PURPLE_BLUE,
	NTSC_BLUE1,
	NTSC_BLUE2,
	NTSC_LIGHT_BLUE,
	NTSC_TURQUOISE,
	NTSC_GREEN_BLUE,
	NTSC_GREEN,
	NTSC_YELLOW_GREEN,
	NTSC_ORANGE_GREEN,
	NTSC_LIGHT_ORANGE
};

static const rgb_t a7800p_palette[256*3] =
{
	NTSC_GREY,
	NTSC_ORANGE_GREEN,
	NTSC_GOLD,
	NTSC_ORANGE,
	NTSC_RED_ORANGE,
	NTSC_PINK,
	NTSC_PURPLE,
	NTSC_PURPLE_BLUE,
	NTSC_BLUE1,
	NTSC_BLUE1,
	NTSC_BLUE2,
	NTSC_LIGHT_BLUE,
	NTSC_TURQUOISE,
	NTSC_GREEN_BLUE,
	NTSC_GREEN,
	NTSC_YELLOW_GREEN
};


/* Initialise the palette */
static PALETTE_INIT(a7800)
{
	palette_set_colors(machine, 0, a7800_palette, ARRAY_LENGTH(a7800_palette));
}


static PALETTE_INIT(a7800p)
{
	palette_set_colors(machine, 0, a7800p_palette, ARRAY_LENGTH(a7800p_palette));
}


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static MACHINE_DRIVER_START( a7800_ntsc )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", M6502, A7800_NTSC_Y1/8)	/* 1.79 MHz (switches to 1.19 MHz on TIA or RIOT access) */
	MDRV_CPU_PROGRAM_MAP(a7800_mem, 0)
	MDRV_CPU_VBLANK_INT_HACK(a7800_interrupt, 262)

	MDRV_MACHINE_RESET(a7800)

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640,262)
	MDRV_SCREEN_VISIBLE_AREA(0,319,25,45+204)
	MDRV_PALETTE_LENGTH(ARRAY_LENGTH(a7800_palette))
	MDRV_PALETTE_INIT(a7800)

	MDRV_VIDEO_START(a7800)
	MDRV_VIDEO_UPDATE(a7800)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("tia", TIA, 31400)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MDRV_SOUND_ADD("pokey", POKEY, A7800_NTSC_Y1/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_RIOT6532_ADD("riot", A7800_NTSC_Y1/12, r6532_interface)

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("a78")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_START(a7800_cart)
	MDRV_CARTSLOT_LOAD(a7800_cart)
	MDRV_CARTSLOT_PARTIALHASH(a7800_partialhash)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( a7800_pal )
	MDRV_IMPORT_FROM( a7800_ntsc )

	/* basic machine hardware */
	MDRV_CPU_REPLACE("main", M6502, CLK_PAL)
	MDRV_CPU_VBLANK_INT_HACK(a7800_interrupt, 312)

	MDRV_SCREEN_MODIFY( "main" )
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_SIZE(640,312)
	MDRV_SCREEN_VISIBLE_AREA(0,319,50,50+225)
	MDRV_PALETTE_INIT( a7800p )

	/* sound hardware */
	MDRV_SOUND_REPLACE("pokey", POKEY, CLK_PAL)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_RIOT6532_REMOVE("riot")
	MDRV_RIOT6532_ADD("riot", 3546894/3, r6532_interface)
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( a7800 )
    ROM_REGION(0x30000, "main", 0)
	ROM_FILL(0x0000, 0x30000, 0xff)
    ROM_SYSTEM_BIOS( 0, "a7800", "Atari 7800" )
    ROMX_LOAD("7800.u7", 0xf000, 0x1000, CRC(5d13730c) SHA1(d9d134bb6b36907c615a594cc7688f7bfcef5b43), ROM_BIOS(1))
    ROM_SYSTEM_BIOS( 1, "a7800pr", "Atari 7800 (prototype with Asteroids)" )
    ROMX_LOAD("c300558-001a.u7", 0xc000, 0x4000, CRC(a0e10edf) SHA1(14584b1eafe9721804782d4b1ac3a4a7313e455f), ROM_BIOS(2))
ROM_END

ROM_START( a7800p )
    ROM_REGION(0x30000, "main", 0)
	ROM_FILL(0x0000, 0x30000, 0xff)
    ROM_LOAD("7800pal.rom", 0xc000, 0x4000, CRC(d5b61170) SHA1(5a140136a16d1d83e4ff32a19409ca376a8df874))
ROM_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME      PARENT    COMPAT  MACHINE     INPUT     INIT          CONFIG      COMPANY   FULLNAME */
CONS( 1986, a7800,    0,        0,		a7800_ntsc,	a7800,    a7800_ntsc,	0,		"Atari",  "Atari 7800 NTSC" , 0)
CONS( 1986, a7800p,   a7800,    0,		a7800_pal,	a7800,    a7800_pal,	0,		"Atari",  "Atari 7800 PAL" , 0)
