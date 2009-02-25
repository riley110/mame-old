/***************************************************************************

        BK driver by Miodrag Milanovic

        10/03/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/t11/t11.h"
#include "sound/wave.h"
#include "includes/bk.h"
#include "devices/cassette.h"
#include "formats/rk_cas.h"

  /* Address maps */
static ADDRESS_MAP_START(bk0010_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x3fff ) AM_RAM  // RAM
	AM_RANGE( 0x4000, 0x7fff ) AM_RAM  AM_BASE(&bk0010_video_ram) // Video RAM
    AM_RANGE( 0x8000, 0x9fff ) AM_ROM  // ROM
    AM_RANGE( 0xa000, 0xbfff ) AM_ROM  // ROM
    AM_RANGE( 0xc000, 0xdfff ) AM_ROM  // ROM
    AM_RANGE( 0xe000, 0xfeff ) AM_ROM  // ROM
    AM_RANGE( 0xffb0, 0xffb1 ) AM_READWRITE (bk_key_state_r,bk_key_state_w)
    AM_RANGE( 0xffb2, 0xffb3 ) AM_READ (bk_key_code_r)
    AM_RANGE( 0xffb4, 0xffb5 ) AM_READWRITE (bk_vid_scrool_r,bk_vid_scrool_w)
    AM_RANGE( 0xffce, 0xffcf ) AM_READWRITE (bk_key_press_r,bk_key_press_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(bk0010fd_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x3fff ) AM_RAM  // RAM
	AM_RANGE( 0x4000, 0x7fff ) AM_RAM  AM_BASE(&bk0010_video_ram) // Video RAM
    AM_RANGE( 0x8000, 0x9fff ) AM_ROM  // ROM
    AM_RANGE( 0xa000, 0xdfff ) AM_RAM  // RAM
    AM_RANGE( 0xe000, 0xfdff ) AM_ROM  // ROM
    AM_RANGE( 0xfe58, 0xfe59 ) AM_READWRITE (bk_floppy_cmd_r,bk_floppy_cmd_w)
    AM_RANGE( 0xfe5a, 0xfe5b ) AM_READWRITE (bk_floppy_data_r,bk_floppy_data_w)
    AM_RANGE( 0xffb0, 0xffb1 ) AM_READWRITE (bk_key_state_r,bk_key_state_w)
    AM_RANGE( 0xffb2, 0xffb3 ) AM_READ (bk_key_code_r)
    AM_RANGE( 0xffb4, 0xffb5 ) AM_READWRITE (bk_vid_scrool_r,bk_vid_scrool_w)
    AM_RANGE( 0xffce, 0xffcf ) AM_READWRITE (bk_key_press_r,bk_key_press_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( bk0010 )
	PORT_START("LINE0")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Clear") PORT_CODE(KEYCODE_HOME)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Rus") PORT_CODE(KEYCODE_LALT)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Lat") PORT_CODE(KEYCODE_RALT)
	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab left") PORT_CODE(KEYCODE_PGUP)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab right") PORT_CODE(KEYCODE_PGDN)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("LINE3")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Delete") PORT_CODE(KEYCODE_BACKSPACE)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("LINE4")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0xFE, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("LINE5")
		PORT_BIT(0xFF, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START("LINE6")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_START("LINE7")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("'") PORT_CODE(KEYCODE_QUOTE)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
	PORT_START("LINE8")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("<>") PORT_CODE(KEYCODE_END)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_START("LINE9")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_START("LINE10")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_START("LINE11")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("~") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_EQUALS)
INPUT_PORTS_END

static const struct t11_setup t11_data =
{
	0x36ff			/* initial mode word has DAL15,14,11,8 pulled low */
};

/* Machine driver */
static const cassette_config bk0010_cassette_config =
{
	/*rk8_cassette_formats*/cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_ENABLED
};


static MACHINE_DRIVER_START( bk0010 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", T11, 3000000)
	MDRV_CPU_CONFIG(t11_data)
    MDRV_CPU_PROGRAM_MAP(bk0010_mem, 0)
    MDRV_MACHINE_RESET( bk0010 )

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 512-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(bk0010)
    MDRV_VIDEO_UPDATE(bk0010)

 	MDRV_SPEAKER_STANDARD_MONO("mono")
   	MDRV_SOUND_ADD("cassette", WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_CASSETTE_ADD( "cassette", bk0010_cassette_config )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bk0010fd )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", T11, 3000000)
	MDRV_CPU_CONFIG(t11_data)
    MDRV_CPU_PROGRAM_MAP(bk0010fd_mem, 0)
    MDRV_MACHINE_RESET( bk0010 )

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 512-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(bk0010)
    MDRV_VIDEO_UPDATE(bk0010)

 	MDRV_SPEAKER_STANDARD_MONO("mono")
   	MDRV_SOUND_ADD("cassette", WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_CASSETTE_ADD( "cassette", bk0010_cassette_config )
MACHINE_DRIVER_END


/* ROM definition */

ROM_START( bk0010 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "monit10.rom", 0x8000, 0x2000, CRC(26c6e8a0) SHA1(4e83a94ae5155bbea14d7331a5a8db82457bd5ae) )
    ROM_LOAD( "focal.rom",   0xa000, 0x2000, CRC(717149b7) SHA1(75df26f81ebd281bcb5c55ba81a7d97f31e388b2) )
    ROM_LOAD( "tests.rom",   0xe000, 0x1f80, CRC(91aecb4d) SHA1(6b14d552045194a3004bb6b795a2538110921519) )
ROM_END

ROM_START( bk001001 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "monit10.rom",   0x8000, 0x2000, CRC(26c6e8a0) SHA1(4e83a94ae5155bbea14d7331a5a8db82457bd5ae) )
    ROM_LOAD( "basic10-1.rom", 0xa000, 0x2000, CRC(5e3ff5da) SHA1(5ea4db1eaac87bd0ac96e52a608bc783709f5042) )
    ROM_LOAD( "basic10-2.rom", 0xc000, 0x2000, CRC(ea63863c) SHA1(acf068925e4052989b05dd5cf736a1dab5438011) )
    ROM_LOAD( "basic10-3.rom", 0xe000, 0x1f80, CRC(63f3df2e) SHA1(b5463f08e7c5f9f5aa31a2e7b6c1ed94fe029d65) )
ROM_END

ROM_START( bk0010fd )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "monit10.rom",  0x8000, 0x2000, CRC(26c6e8a0) SHA1(4e83a94ae5155bbea14d7331a5a8db82457bd5ae) )
    ROM_LOAD( "disk_327.rom", 0xe000, 0x1000, CRC(ed8a43ae) SHA1(28eefbb63047b26e4aec104aeeca74e2f9d0276c) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT       INIT     CONFIG COMPANY                  FULLNAME   FLAGS */
COMP( 1985, bk0010, 	 0,  	 0,	bk0010, 	bk0010, 	bk0010, 0,  "Elektronika",			 "BK-0010",	 0)
COMP( 1986, bk001001, 	bk0010,  0,	bk0010, 	bk0010, 	bk0010, 0,  "Elektronika",			 "BK-0010.01",	 0)
COMP( 1986, bk0010fd, 	bk0010,  0,	bk0010fd, 	bk0010, 	bk0010, 0,  "Elektronika",			 "BK-0010 FDD",	 GAME_NOT_WORKING)


