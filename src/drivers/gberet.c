/***************************************************************************

Green Beret memory map (preliminary)

0000-bfff ROM
c000-c7ff Color RAM
c800-cfff Video RAM
d000-d0c0 Sprites (bank 0)
d100-d1c0 Sprites (bank 1)
d200-dfff RAM
e000-e01f ZRAM1 line scroll registers
e020-e03f ZRAM2 bit 8 of line scroll registers

read:
f200      DSW1
          bit 0-1 lives
          bit 2   cocktail/upright cabinet (0 = upright)
          bit 3-4 bonus
          bit 5-6 difficulty
          bit 7   demo sounds
f400      DSW2
          bit 0 = screen flip
          bit 1 = single/dual upright controls
f600      DSW0
          bit 0-1-2-3 coins per play Coin1
          bit 4-5-6-7 coins per play Coin2
f601      IN1 player 2 controls
f602      IN0 player 1 controls
f603      IN2
          bit 0-1-2 coin  bit 3 1 player start  bit 4 2 players start

write:
e040      ?
e041      ?
e042      ?
e043      bit 3 = sprite RAM bank select; other bits = ?
e044      bit 0 = nmi enable, bit 3 = flip screen, other bits = ?
f000      ?
f200      SN76496 command
f400      SN76496 trigger (write command to f200, then write to this location
          to cause the chip to read it)
f600      watchdog reset (?)

interrupts:
The game uses both IRQ (mode 1) and NMI.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *gberet_scroll;
extern unsigned char *gberet_spritebank;
void gberet_e044_w(int offset,int data);
void gberet_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int gberet_vh_start(void);
void gberet_vh_stop(void);
void gberet_vh_screenrefresh(struct osd_bitmap *bitmap);

int gberet_interrupt(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xe03f, MRA_RAM },
	{ 0xf200, 0xf200, input_port_4_r },	/* DSW1 */
	{ 0xf400, 0xf400, input_port_5_r },	/* DSW2 */
	{ 0xf600, 0xf600, input_port_3_r },	/* DSW0 */
	{ 0xf601, 0xf601, input_port_1_r },	/* IN1 */
	{ 0xf602, 0xf602, input_port_0_r },	/* IN0 */
	{ 0xf603, 0xf603, input_port_2_r },	/* IN2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, colorram_w, &colorram },
	{ 0xc800, 0xcfff, videoram_w, &videoram, &videoram_size },
	{ 0xd000, 0xd0bf, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd100, 0xd1bf, MWA_RAM, &spriteram_2 },
	{ 0xd200, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe03f, MWA_RAM, &gberet_scroll },
	{ 0xe043, 0xe043, MWA_RAM, &gberet_spritebank },
	{ 0xe044, 0xe044, gberet_e044_w },
	{ 0xf000, 0xf000, MWA_NOP },
	{ 0xf200, 0xf200, MWA_NOP },
	{ 0xf400, 0xf400, SN76496_0_w },
	{ 0xf600, 0xf600, MWA_NOP },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x50, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x10, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x70, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	/* 0x00 is invalid */

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x18, 0x18, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "30000 70000" )
	PORT_DIPSETTING(    0x10, "40000 80000" )
	PORT_DIPSETTING(    0x08, "50000 100000" )
	PORT_DIPSETTING(    0x00, "50000 200000" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Controls", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Single" )
	PORT_DIPSETTING(    0x00, "Dual" )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
		32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		64*8+0*32, 64*8+1*32, 64*8+2*32, 64*8+3*32, 64*8+4*32, 64*8+5*32, 64*8+6*32, 64*8+7*32 },
	128*8	/* every sprite takes 128 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   16*16, 16 },
	{ 1, 0x04000, &spritelayout,     0, 16 },
	{ 1, 0x0c000, &spritelayout,     0, 16 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* 577h09 - palette */
	0x00,0x1A,0x26,0x1C,0xB6,0x74,0x0A,0x52,0xA4,0xD0,0xE8,0xAD,0x3F,0x06,0xFF,0x40,
	0x00,0x05,0x04,0x02,0x88,0x37,0xAA,0xAC,0x16,0xAE,0x24,0x10,0x52,0xA4,0xF6,0xFF,
	/* 577h10 - sprites */
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x00,0x0D,0x0C,0x0F,0x00,0x0D,0x0C,0x0F,0x00,0x0D,0x0C,0x0F,0x00,0x0D,0x0C,0x0F,
	0x00,0x01,0x03,0x03,0x03,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x02,0x0F,
	0x00,0x06,0x02,0x04,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x00,0x01,0x00,0x03,0x04,0x05,0x06,0x0E,0x00,0x09,0x0A,0x0B,0x07,0x0D,0x00,0x0F,
	0x00,0x01,0x02,0x0B,0x04,0x05,0x01,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x00,0x01,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0C,0x0D,0x0F,0x0F,
	0x00,0x01,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x00,0x01,0x01,0x01,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x04,0x0F,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0D,0x0B,0x0E,0x0F,
	0x00,0x0D,0x00,0x00,0x00,0x0D,0x0D,0x0D,0x0D,0x00,0x0D,0x0D,0x0D,0x00,0x00,0x00,
	0x00,0x05,0x00,0x00,0x00,0x05,0x05,0x05,0x05,0x00,0x05,0x05,0x05,0x00,0x00,0x00,
	0x00,0x00,0x02,0x03,0x04,0x09,0x06,0x04,0x02,0x09,0x0D,0x0E,0x0F,0x0D,0x0E,0x0F,
	0x00,0x00,0x02,0x03,0x04,0x09,0x06,0x07,0x08,0x09,0x0D,0x0E,0x0F,0x0D,0x0E,0x0F,
	0x00,0x0E,0x00,0x00,0x00,0x0E,0x0E,0x0E,0x0E,0x00,0x0E,0x0E,0x0E,0x00,0x00,0x00,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0D,0x0A,0x0E,0x0F,
	/* 577h11 - characters */
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x07,0x07,0x07,0x07,0x0E,0x0E,0x0E,0x0E,0x0D,0x0D,0x0D,0x0D,0x0C,0x0C,0x0C,0x0C,
	0x07,0x0F,0x0E,0x0D,0x07,0x0F,0x0E,0x0D,0x07,0x0F,0x0E,0x0D,0x07,0x0F,0x0E,0x0D,
	0x00,0x00,0x00,0x00,0x0E,0x0E,0x0E,0x0E,0x0D,0x0D,0x0D,0x0D,0x0C,0x0C,0x0C,0x0C,
	0x00,0x01,0x02,0x03,0x00,0x01,0x02,0x03,0x00,0x01,0x02,0x03,0x00,0x01,0x02,0x03,
	0x00,0x01,0x0B,0x08,0x00,0x01,0x0B,0x08,0x00,0x01,0x0B,0x08,0x00,0x01,0x0B,0x08,
	0x07,0x07,0x07,0x07,0x01,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0x03,0x03,0x03,0x03,
	0x00,0x0D,0x04,0x0C,0x00,0x0D,0x04,0x0C,0x00,0x0D,0x04,0x0C,0x00,0x0D,0x04,0x0C,
	0x00,0x0D,0x0A,0x0B,0x00,0x0D,0x0A,0x0B,0x00,0x0D,0x0A,0x0B,0x00,0x0D,0x0A,0x0B,
	0x00,0x06,0x07,0x0C,0x00,0x06,0x07,0x0C,0x00,0x06,0x07,0x0C,0x00,0x06,0x07,0x0C,
	0x00,0x0F,0x0E,0x0D,0x00,0x0F,0x0E,0x0D,0x00,0x0F,0x0E,0x0D,0x00,0x0F,0x0E,0x0D,
	0x00,0x0E,0x0D,0x0C,0x00,0x0E,0x0D,0x0C,0x00,0x0E,0x0D,0x0C,0x00,0x0E,0x0D,0x0C,
	0x00,0x00,0x00,0x00,0x0A,0x0A,0x0A,0x0A,0x0B,0x0B,0x0B,0x0B,0x0C,0x0C,0x0C,0x0C,
	0x0D,0x0D,0x0D,0x0D,0x0E,0x0E,0x0E,0x0E,0x0D,0x0D,0x0D,0x0D,0x0C,0x0C,0x0C,0x0C,
	0x00,0x05,0x08,0x0E,0x00,0x05,0x08,0x0E,0x00,0x05,0x08,0x0E,0x00,0x05,0x08,0x0E,
	0x00,0x08,0x03,0x09,0x00,0x08,0x03,0x09,0x00,0x08,0x03,0x09,0x00,0x08,0x03,0x09
};



static struct SN76496interface sn76496_interface =
{
	1,	/* 1 chip */
	1500000,	/* hand tuned, however other sources claim that */
				/* the schematics mention a 1.78975 MHz freq) */
	{ 255 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 MHz ?? */
			0,
			readmem,writemem,0,0,
			gberet_interrupt,32	/* 1 IRQ + 16 NMI */
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32,2*16*16,
	gberet_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	gberet_vh_start,
	gberet_vh_stop,
	gberet_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gberet_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "c10_l03.bin", 0x0000, 0x4000, 0xe77699e2 )
	ROM_LOAD( "c08_l02.bin", 0x4000, 0x4000, 0x6c2df979 )
	ROM_LOAD( "c07_l01.bin", 0x8000, 0x4000, 0x96c963ab )

	ROM_REGION(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "f03_l07.bin", 0x00000, 0x4000, 0x7f70cdc4 )
	ROM_LOAD( "e05_l06.bin", 0x04000, 0x4000, 0x023e4fbc )
	ROM_LOAD( "e04_l05.bin", 0x08000, 0x4000, 0x9496cc60 )
	ROM_LOAD( "f04_l08.bin", 0x0c000, 0x4000, 0x69b3d78b )
	ROM_LOAD( "e03_l04.bin", 0x10000, 0x4000, 0x6f1e4796 )
ROM_END

ROM_START( rushatck_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "rush_h03.10c", 0x0000, 0x4000, 0xf32c1d1e )
	ROM_LOAD( "rush_h02.8c",  0x4000, 0x4000, 0xea94b072 )
	ROM_LOAD( "rush_h01.7c",  0x8000, 0x4000, 0x9579d71f )

	ROM_REGION(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "rush_h07.3f",  0x00000, 0x4000, 0x42cc0d1a )
	ROM_LOAD( "rush_h06.5e",  0x04000, 0x4000, 0x023e4fbc )
	ROM_LOAD( "rush_h05.4e",  0x08000, 0x4000, 0x4b711f89 )
	ROM_LOAD( "rush_h08.4f",  0x0c000, 0x4000, 0x69b3d78b )
	ROM_LOAD( "rush_h04.3e",  0x10000, 0x4000, 0x6f1e4796 )
ROM_END



static int hiload(void)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xd900],"\x03\x30\x00",3) == 0 &&
			memcmp(&RAM[0xd91b],"\x01\x00\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xd900],6*10);
			RAM[0xdb06] = RAM[0xd900];
			RAM[0xdb07] = RAM[0xd901];
			RAM[0xdb08] = RAM[0xd902];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xd900],6*10);
		osd_fclose(f);
	}
}



struct GameDriver gberet_driver =
{
	"Green Beret",
	"gberet",
	"Nicola Salmoria (MAME driver)\nChris Hardy (hardware info)\nPaul Swan (color info)\nMarco Cassili",
	&machine_driver,

	gberet_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver rushatck_driver =
{
	"Rush'n Attack",
	"rushatck",
	"Nicola Salmoria (MAME driver)\nChris Hardy (hardware info)\nPaul Swan (color info)\nMarco Cassili",
	&machine_driver,

	rushatck_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
