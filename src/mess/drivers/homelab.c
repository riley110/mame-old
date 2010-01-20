/***************************************************************************

        Homelab driver by Miodrag Milanovic

        31/08/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/homelab.h"

static WRITE8_HANDLER (homelab_keyboard_w)
{
}

static READ8_HANDLER (homelab_keyboard_r)
{
	UINT8 key_mask = (offset ^ 0xff) & 0xff;
	UINT8 key = 0xff;
	if ((key_mask & 0x01)!=0) { key &= input_port_read(space->machine,"LINE0"); }
	if ((key_mask & 0x02)!=0) { key &= input_port_read(space->machine,"LINE1"); }
	if ((key_mask & 0x04)!=0) { key &= input_port_read(space->machine,"LINE2"); }
	if ((key_mask & 0x08)!=0) { key &= input_port_read(space->machine,"LINE3"); }
	if ((key_mask & 0x10)!=0) { key &= input_port_read(space->machine,"LINE4"); }
	if ((key_mask & 0x20)!=0) { key &= input_port_read(space->machine,"LINE5"); }
	if ((key_mask & 0x40)!=0) { key &= input_port_read(space->machine,"LINE6"); }
	if ((key_mask & 0x80)!=0) { key &= input_port_read(space->machine,"LINE7"); }
	return key;
}

/* Address maps */
static ADDRESS_MAP_START(homelab2_mem, ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE( 0x0000, 0x07ff ) AM_ROM  // ROM 1
    AM_RANGE( 0x0800, 0x0fff ) AM_ROM  // ROM 2
    AM_RANGE( 0x1000, 0x17ff ) AM_ROM  // ROM 3
    AM_RANGE( 0x1800, 0x1fff ) AM_ROM  // ROM 4
    AM_RANGE( 0x2000, 0x27ff ) AM_ROM  // ROM 5
    AM_RANGE( 0x2800, 0x2fff ) AM_ROM  // ROM 6
	AM_RANGE( 0x3000, 0x37ff ) AM_ROM  // Empty
	AM_RANGE( 0x3800, 0x3fff ) AM_READWRITE(homelab_keyboard_r,homelab_keyboard_w)
    AM_RANGE( 0x4000, 0x7fff ) AM_RAM
    AM_RANGE( 0xc000, 0xc3ff ) AM_RAM AM_MIRROR(0x3c00) // Video RAM 1K
ADDRESS_MAP_END

static ADDRESS_MAP_START(homelab3_mem, ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE( 0x0000, 0x3fff ) AM_ROM
    AM_RANGE( 0x4000, 0x7fff ) AM_RAM
    //AM_RANGE( 0xe000, 0xefff ) AM_RAM // Keyboard
    AM_RANGE( 0xf000, 0xf7ff ) AM_RAM AM_MIRROR(0x0800)// Video RAM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( homelab )
	PORT_START("LINE0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left Shift")  PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0xFE, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT)

	PORT_START("LINE1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Home") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Run/Brk") PORT_CODE(KEYCODE_RCONTROL)

	PORT_START("LINE2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('<')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')

	PORT_START("LINE3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR('<') PORT_CHAR(',')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('=') PORT_CHAR('-')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('>') PORT_CHAR('.')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('?') PORT_CHAR('/')

	PORT_START("LINE4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('@')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START("LINE5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START("LINE6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START("LINE7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('^')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RALT) PORT_CODE(KEYCODE_LALT) PORT_CHAR('_')
INPUT_PORTS_END

/* F4 Character Displayer */
static const gfx_layout homelab_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0, 1*256*8, 2*256*8, 3*256*8, 4*256*8, 5*256*8, 6*256*8, 7*256*8 },
	8					/* every char takes 8 x 1 bytes */
};

static GFXDECODE_START( homelab )
	GFXDECODE_ENTRY( "gfx1", 0x0000, homelab_charlayout, 0, 1 )
GFXDECODE_END


/* Machine driver */
static MACHINE_DRIVER_START( homelab )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 3000000)
	MDRV_CPU_PROGRAM_MAP(homelab2_mem)
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)

	/* video hardware */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(40*8, 25*8)
	MDRV_SCREEN_VISIBLE_AREA(0, 40*8-1, 0, 25*8-1)
	MDRV_GFXDECODE(homelab)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT( black_and_white )

	MDRV_VIDEO_START( homelab )
	MDRV_VIDEO_UPDATE( homelab )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( homelab3 )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(homelab)
  	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(homelab3_mem)

	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_SIZE(80*8, 25*8)
	MDRV_SCREEN_VISIBLE_AREA(0, 80*8-1, 0, 25*8-1)
	MDRV_VIDEO_UPDATE( homelab3 )

MACHINE_DRIVER_END

/* ROM definition */

ROM_START( homelab2 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "hl2_1.rom", 0x0000, 0x0800, CRC(205365F7) SHA1(da93b65befd83513dc762663b234227ba804124d))
	ROM_LOAD( "hl2_2.rom", 0x0800, 0x0800, CRC(696AF3C1) SHA1(b53bc6ae2b75975618fc90e7181fa5d21409fce1))
	ROM_LOAD( "hl2_3.rom", 0x1000, 0x0800, CRC(69E57E8C) SHA1(e98510abb715dbf513e1b29fb6b09ab54e9483b7))
  	ROM_LOAD( "hl2_4.rom", 0x1800, 0x0800, CRC(97CBBE74) SHA1(34f0bad41302b059322018abc3d1c2336ecfbea8))
	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD ("hl2.chr", 0x0000, 0x0800, CRC(2E669D40) SHA1(639dd82ed29985dc69830aca3b904b6acc8fe54a))
ROM_END

ROM_START( homelab3 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "hl3_1.rom", 0x0000, 0x1000, CRC(6B90A8EA) SHA1(8ac40ca889b8c26cdf74ca309fbafd70dcfdfbec))
	ROM_LOAD( "hl3_2.rom", 0x1000, 0x1000, CRC(BCAC3C24) SHA1(aff371d17f61cb60c464998e092f04d5d85c4d52))
	ROM_LOAD( "hl3_3.rom", 0x2000, 0x1000, CRC(AB1B4AB0) SHA1(ad74c7793f5dc22061a88ef31d3407267ad08719))
  	ROM_LOAD( "hl3_4.rom", 0x3000, 0x1000, CRC(BF67EFF9) SHA1(2ef5d46f359616e7d0e5a124df528de44f0e850b))
	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD ("hl3.chr", 0x0000, 0x0800, CRC(F58EE39B) SHA1(49399c42d60a11b218a225856da86a9f3975a78a))
ROM_END

ROM_START( homelab4 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "hl4_1.rom", 0x0000, 0x1000, CRC(A549B2D4) SHA1(90fc5595da8431616aee56eb5143b9f04281e798))
	ROM_LOAD( "hl4_2.rom", 0x1000, 0x1000, CRC(151D33E8) SHA1(d32004bc1553f802b9d3266709552f7d5315fe44))
	ROM_LOAD( "hl4_3.rom", 0x2000, 0x1000, CRC(39571AB1) SHA1(8470cff2e3442101e6a0bc655358b3a6fc1ef944))
  	ROM_LOAD( "hl4_4.rom", 0x3000, 0x1000, CRC(F4B77CA2) SHA1(ffbdb3c1819c7357e2a0fc6317c111a8a7ecfcd5))
	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD ("hl4.chr", 0x0000, 0x0800, CRC(F58EE39B) SHA1(49399c42d60a11b218a225856da86a9f3975a78a))
ROM_END

ROM_START( brailab4 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "brl1.rom", 0x0000, 0x1000, CRC(02323403) SHA1(3a2e853e0a39e05a04a8db58e1a76de1eda579c9))
	ROM_LOAD( "brl2.rom", 0x1000, 0x1000, CRC(36173fbc) SHA1(1c01398e16a1cbe4103e1be769347ceae873e090))
	ROM_LOAD( "brl3.rom", 0x2000, 0x1000, CRC(d3cdd108) SHA1(1a24e6c5f9c370ff6cb25045cb9d95e664467eb5))
	ROM_LOAD( "brl4.rom", 0x3000, 0x1000, CRC(d4047885) SHA1(00fe40c4c2c64a49bb429fb2b27cc7e0d0025a85))
	ROM_LOAD( "brl5.rom", 0x4000, 0x1000, CRC(8a76be04) SHA1(4b683b9be23b47117901fe874072eb7aa481e4ff))
	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD ("hl4.chr", 0x0000, 0x0800, CRC(F58EE39B) SHA1(49399c42d60a11b218a225856da86a9f3975a78a))
ROM_END

/* Driver */

/*    YEAR  NAME   PARENT  COMPAT  MACHINE  INPUT   INIT  				 COMPANY                 FULLNAME   FLAGS */
COMP( 1982, homelab2,   0	,	 0, 	homelab, 	homelab, 	homelab, "Jozsef and Endre Lukacs", "Homelab 2 / Aircomp 16",		 GAME_NOT_WORKING)
COMP( 1983, homelab3,   homelab2,0, 	homelab3, 	homelab, 	homelab, "Jozsef and Endre Lukacs", "Homelab 3",		 GAME_NOT_WORKING)
COMP( 1984, homelab4,   homelab2,0, 	homelab3, 	homelab, 	homelab, "Jozsef and Endre Lukacs", "Homelab 4",		 GAME_NOT_WORKING)
COMP( 1984, brailab4,   homelab2,0, 	homelab3, 	homelab, 	homelab, "Jozsef and Endre Lukacs", "Brailab 4",		 GAME_NOT_WORKING)
