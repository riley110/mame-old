/************************************************************************
Philips P2000 1 Memory map

	CPU: Z80
		0000-0fff	ROM
		1000-4fff	ROM (appl)
		5000-57ff	RAM (Screen T ver)
		5000-5fff	RAM (Screen M ver)
		6000-9fff	RAM (system)
		a000-ffff	RAM (extension)

	Interrupts:

	Ports:
		00-09		Keyboard input
		10-1f		Output ports
		20-2f		Input ports
		30-3f		Scroll reg (T ver)
		50-5f		Beeper
		70-7f		DISAS (M ver)
		88-8B		CTC
		8C-90		Floppy ctrl
		94			RAM Bank select

	Display: SAA5050

************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "includes/saa5050.h"
#include "includes/p2000t.h"

/* port i/o functions */

static ADDRESS_MAP_START( p2000t_readport , ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x00, 0x0f) AM_READ( p2000t_port_000f_r)
	AM_RANGE(0x20, 0x2f) AM_READ( p2000t_port_202f_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( p2000t_writeport , ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x10, 0x1f) AM_WRITE( p2000t_port_101f_w)
	AM_RANGE(0x30, 0x3f) AM_WRITE( p2000t_port_303f_w)
	AM_RANGE(0x50, 0x5f) AM_WRITE( p2000t_port_505f_w)
	AM_RANGE(0x70, 0x7f) AM_WRITE( p2000t_port_707f_w)
	AM_RANGE(0x88, 0x8b) AM_WRITE( p2000t_port_888b_w)
	AM_RANGE(0x8c, 0x90) AM_WRITE( p2000t_port_8c90_w)
	AM_RANGE(0x94, 0x94) AM_WRITE( p2000t_port_9494_w)
ADDRESS_MAP_END

/* Memory w/r functions */

static ADDRESS_MAP_START( p2000t_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_READ( MRA8_ROM)
	AM_RANGE(0x1000, 0x4fff) AM_READ( MRA8_RAM)
	AM_RANGE(0x5000, 0x57ff) AM_READ( videoram_r)
	AM_RANGE(0x5800, 0x9fff) AM_READ( MRA8_RAM)
	AM_RANGE(0xa000, 0xffff) AM_READ( MRA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( p2000t_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_WRITE( MWA8_ROM)
	AM_RANGE(0x1000, 0x4fff) AM_WRITE( MWA8_RAM)
	AM_RANGE(0x5000, 0x57ff) AM_WRITE( videoram_w) AM_BASE( &videoram) AM_SIZE( &videoram_size)
	AM_RANGE(0x5800, 0x9fff) AM_WRITE( MWA8_RAM)
	AM_RANGE(0xa000, 0xffff) AM_WRITE( MWA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( p2000m_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_READ( MRA8_ROM)
	AM_RANGE(0x1000, 0x4fff) AM_READ( MRA8_RAM)
	AM_RANGE(0x5000, 0x5fff) AM_READ( videoram_r)
	AM_RANGE(0x6000, 0x9fff) AM_READ( MRA8_RAM)
	AM_RANGE(0xa000, 0xffff) AM_READ( MRA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( p2000m_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_WRITE( MWA8_ROM)
	AM_RANGE(0x1000, 0x4fff) AM_WRITE( MWA8_RAM)
	AM_RANGE(0x5000, 0x5fff) AM_WRITE( videoram_w) AM_BASE( &videoram) AM_SIZE( &videoram_size)
	AM_RANGE(0x6000, 0x9fff) AM_WRITE( MWA8_RAM)
	AM_RANGE(0xa000, 0xffff) AM_WRITE( MWA8_NOP)
ADDRESS_MAP_END

/* graphics output */

struct	GfxLayout p2000m_charlayout =
{
	6, 10,
	256,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8,
	  5*8, 6*8, 7*8, 8*8, 9*8 },
	8 * 10
};

static	struct	GfxDecodeInfo p2000m_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &p2000m_charlayout, 0, 128 },
	{ -1 }
};

static	unsigned	char	p2000m_palette[2 * 3] =
{
	0x00, 0x00, 0x00,
	0xff, 0xff, 0xff
};

static	unsigned	short	p2000m_colortable[2 * 2] =
{
	1,0, 0,1
};

static PALETTE_INIT( p2000m )
{
	palette_set_colors(0, p2000m_palette, sizeof(p2000m_palette) / 3);
	memcpy(colortable, p2000m_colortable, sizeof (p2000m_colortable));
}

/* Keyboard input */

INPUT_PORTS_START (p2000t)
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Left", KEYCODE_LEFT, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Up", KEYCODE_UP, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Tab", KEYCODE_TAB, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad .", KEYCODE_DEL_PAD, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Space", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad 00", KEYCODE_ENTER_PAD, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad 0", KEYCODE_0_PAD, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "#", KEYCODE_BACKSLASH, IP_JOY_NONE)
	PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Down", KEYCODE_DOWN, IP_JOY_NONE)
	PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Right", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Shift Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "<", KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Code", KEYCODE_HOME, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Clrln", KEYCODE_END, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad +", KEYCODE_PLUS_PAD, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad -", KEYCODE_MINUS_PAD, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Backspace", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad 9", KEYCODE_9_PAD, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad 8", KEYCODE_8_PAD, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad 7", KEYCODE_7_PAD, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Enter", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad 3", KEYCODE_3_PAD, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad 2", KEYCODE_2_PAD, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad 1", KEYCODE_1_PAD, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "->", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad 6", KEYCODE_6_PAD, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad 5", KEYCODE_5_PAD, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Pad 4", KEYCODE_4_PAD, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "1/4", KEYCODE_TILDE, IP_JOY_NONE)
	PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_COLON, IP_JOY_NONE)
	PORT_START
	PORT_BITX (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "LShift", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "RShift", KEYCODE_RSHIFT, IP_JOY_NONE)
INPUT_PORTS_END

/* Sound output */

static struct Speaker_interface speaker_interface =
{
	1,			/* one speaker */
	{ 100 },	/* mixing levels */
	{ 0 },		/* optional: number of different levels */
    { NULL }    /* optional: level lookup table */
};

static INTERRUPT_GEN( p2000_interrupt )
{
	cpunum_set_input_line(0, 0, PULSE_LINE);
}

/* Machine definition */
static MACHINE_DRIVER_START( p2000t )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 2500000)
	MDRV_CPU_PROGRAM_MAP(p2000t_readmem, p2000t_writemem)
	MDRV_CPU_IO_MAP(p2000t_readport, p2000t_writeport)
	MDRV_CPU_VBLANK_INT(p2000_interrupt, 1)

    /* video hardware */
	MDRV_IMPORT_FROM( vh_saa5050 )

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, speaker_interface)
MACHINE_DRIVER_END


/* Machine definition */
static MACHINE_DRIVER_START( p2000m )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 2500000)
	MDRV_CPU_PROGRAM_MAP(p2000m_readmem, p2000m_writemem)
	MDRV_CPU_IO_MAP(p2000t_readport, p2000t_writeport)
	MDRV_CPU_VBLANK_INT(p2000_interrupt, 1)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(2500)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(80 * 6, 24 * 10)
	MDRV_VISIBLE_AREA(0, 80 * 6 - 1, 0, 24 * 10 - 1)
	MDRV_GFXDECODE( p2000m_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(4)
	MDRV_PALETTE_INIT(p2000m)

	MDRV_VIDEO_START(p2000m)
	MDRV_VIDEO_UPDATE(p2000m)

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, speaker_interface)
MACHINE_DRIVER_END


ROM_START(p2000t)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("p2000.rom", 0x0000, 0x1000, CRC(650784a3))
	ROM_LOAD("basic.rom", 0x1000, 0x4000, CRC(9d9d38f9))
	ROM_REGION(0x01000, REGION_GFX1,0)
	ROM_LOAD("p2000.chr", 0x0140, 0x08c0, BAD_DUMP CRC(78c17e3e))
ROM_END

ROM_START(p2000m)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("p2000.rom", 0x0000, 0x1000, CRC(650784a3))
	ROM_LOAD("basic.rom", 0x1000, 0x4000, CRC(9d9d38f9))
	ROM_REGION(0x01000, REGION_GFX1,0)
	ROM_LOAD("p2000.chr", 0x0140, 0x08c0, BAD_DUMP CRC(78c17e3e))
ROM_END

/*		YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT	CONFIG  COMPANY		FULLNAME */
COMP (	1980,	p2000t,		0,		0,		p2000t,		p2000t,		0,		NULL,	"Philips",	"Philips P2000T" )
COMP (	1980,	p2000m,		p2000t,	0,		p2000m,		p2000t,		0,		NULL,	"Philips",	"Philips P2000M" )

