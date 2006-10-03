
/************************************
 Card Line
 driver by Tomasz Slanina
 analog[at]op[dot]pl


 SIEMENS 80C32 (main cpu)
 MC6845P
 GM76C88 x3 (8K x 8 RAM)
 K-665 9546 (OKI 6295)
 STARS B2072 9629 (qfp ASIC)
 XTAL 12 MHz
 XTAL  4 MHz

***********************************/


#include "driver.h"
#include "cpu/i8051/i8051.h"
#include "sound/okim6295.h"

#define USE_LAMPS
//fake lamps - remove above line to disable

static int cardline_video;
static int cardline_lamps;

#define DRAW_TILE(offset, transparency) drawgfx(bitmap, Machine->gfx[0],\
					(videoram[index+offset] | (colorram[index+offset]<<8))&0x3fff,\
					(colorram[index+offset]&0x80)>>7,\
					0,0,\
					x<<3, y<<3,\
					&Machine->screen[0].visarea,\
					transparency?TRANSPARENCY_PEN:TRANSPARENCY_NONE,transparency);

VIDEO_UPDATE( cardline )
{
	int x,y;
	fillbitmap(bitmap,Machine->pens[0],&Machine->screen[0].visarea);
	for(y=0;y<32;y++)
	{
		for(x=0;x<64;x++)
		{
			int index=y*64+x;
			if(cardline_video&1)
			{
				DRAW_TILE(0,0);
				DRAW_TILE(0x800,1);
			}

			if(cardline_video&2)
			{
				DRAW_TILE(0x1000,0);
				DRAW_TILE(0x1800,1);
			}
		}
	}
#ifdef USE_LAMPS
	{
		//fake lamps (only 5 buttons - there are at least 8 lamps (5 buttons +start +bet +?)
		int i,j,k;
		j=cardline_lamps>>1;
		for(i=0;i<5;i++)
		{
			if(j&1)
			{
				for(k=0;k<10;k++)
				{
					drawgfx(bitmap, Machine->gfx[0],
						0x3fff,
						0,
						0,0,
						104*i+(k+1)*8, 264,
						&Machine->screen[0].visarea,
						TRANSPARENCY_NONE,0);
				}
			}
			j>>=1;
		}
	}
#endif
	return 0;
}

static WRITE8_HANDLER(vram_w)
{
	offset+=0x1000*((cardline_video&2)>>1);
	videoram[offset]=data;
}

static WRITE8_HANDLER(attr_w)
{
	offset+=0x1000*((cardline_video&2)>>1);
	colorram[offset]=data;
}

static WRITE8_HANDLER(video_w)
{
	cardline_video=data;
}

static READ8_HANDLER(unk_r)
{
	static int var=0;
	var^=0x10;
	return var;
}

static WRITE8_HANDLER(lamps_w)
{
	// lamps linked to buttons (check input port 0)
	cardline_lamps=data;
}

static ADDRESS_MAP_START( mem_prg, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mem_data, ADDRESS_SPACE_DATA, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM
	AM_RANGE(0x2003, 0x2003) AM_READ(input_port_0_r)
	AM_RANGE(0x2005, 0x2005) AM_READ(input_port_1_r)
	AM_RANGE(0x2006, 0x2006) AM_READ(input_port_2_r)
	AM_RANGE(0x2007, 0x2007) AM_WRITE(lamps_w)
	AM_RANGE(0x2008, 0x2008) AM_NOP
	AM_RANGE(0x2080, 0x213f) AM_NOP
	AM_RANGE(0x2400, 0x2400) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0x2800, 0x2801) AM_NOP
	AM_RANGE(0x2840, 0x2840) AM_NOP
	AM_RANGE(0x2880, 0x2880) AM_NOP
	AM_RANGE(0x3003, 0x3003) AM_NOP
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(vram_w) AM_BASE(&videoram)
	AM_RANGE(0xe000, 0xffff) AM_WRITE(attr_w) AM_BASE(&colorram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mem_io, ADDRESS_SPACE_IO, 8 )
  AM_RANGE(0x01, 0x01) AM_READWRITE(unk_r, video_w)
ADDRESS_MAP_END

INPUT_PORTS_START( cardline )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Button 1")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Button 2")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_NAME("Button 3")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_NAME("Button 4")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON9 ) PORT_NAME("Button 5")

	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Bet")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bookkeeping Info") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_L) PORT_NAME("Payout 2")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
  PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_CODE(KEYCODE_ENTER) PORT_NAME("Payout")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON10 )

	PORT_START
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf5, IP_ACTIVE_HIGH, IPT_SPECIAL ) // h/w status bits
INPUT_PORTS_END

static gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7  },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*8*8
};

static gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 2 },
	{ -1 }	/* end of array */
};

PALETTE_INIT(cardline)
{
	int i,r,g,b,data;
	int bit0,bit1,bit2;
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		data=color_prom[i];

		/* red component */
		bit0 = (data >> 5) & 0x01;
		bit1 = (data >> 6) & 0x01;
		bit2 = (data >> 7) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (data >> 2) & 0x01;
		bit1 = (data >> 3) & 0x01;
		bit2 = (data >> 4) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (data >> 0) & 0x01;
		bit1 = (data >> 1) & 0x01;
		b = 0x55 * bit0 + 0xaa * bit1;
		palette_set_color(machine,i,r,g,b);
	}
}

static MACHINE_DRIVER_START( cardline )

	/* basic machine hardware */
	MDRV_CPU_ADD(I8051,12000000)
	MDRV_CPU_PROGRAM_MAP(mem_prg,0)
	MDRV_CPU_DATA_MAP(mem_data,0)
	MDRV_CPU_IO_MAP(mem_io,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 35*8)
#ifdef USE_LAMPS
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 35*8-1)
#else
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 32*8-1)
#endif
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)
	MDRV_PALETTE_INIT(cardline)

	MDRV_VIDEO_UPDATE(cardline)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)

MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( cardline )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "dns0401.u23",   0x0000, 0x10000, CRC(5bbaf5c1) SHA1(70972a744c5981b01a46799a7fd1b0a600489264) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "u38cll01.u38",   0x000001, 0x80000, CRC(12f62496) SHA1(b89eaf09e76c5c42588bf9c8c23190347635cc83) )
	ROM_LOAD16_BYTE( "u39cll01.u39",   0x000000, 0x80000, CRC(fcfa703e) SHA1(9230ad9df02140f3a6c38b24558548a888b23412) )

	ROM_REGION( 0x40000,  REGION_SOUND1, 0 ) // OKI samples
	ROM_LOAD( "3a.u3",   0x0000, 0x40000, CRC(9fa543c5) SHA1(a22396cb341ca4a3f0dd23719620a219c91e0e9d) )

	ROM_REGION( 0x0200,  REGION_PROMS, 0 )
	ROM_LOAD( "82s147.u33",   0x0000, 0x0200, CRC(a3b95911) SHA1(46850ea38950cdccbc2ad91d968218ac964c0eb5) )

ROM_END

GAME( 199?, cardline,  0,       cardline,  cardline,  0, ROT0, "Veltmeijer", "Card Line" , 0)
