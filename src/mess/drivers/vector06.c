/***************************************************************************

        Vector06c driver by Miodrag Milanovic

        10/07/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "machine/8255ppi.h"
#include "devices/cassette.h"
#include "devices/cartslot.h"
#include "includes/vector06.h"

/* Address maps */
static ADDRESS_MAP_START(vector06_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x7fff ) AM_READWRITE(SMH_BANK1, SMH_BANK2)
  	AM_RANGE( 0x8000, 0xffff ) AM_READWRITE(SMH_BANK3, SMH_BANK4)
ADDRESS_MAP_END

static ADDRESS_MAP_START( vector06_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x00, 0x03) AM_READWRITE(vector_8255_1_r, vector_8255_1_w )
	AM_RANGE( 0x04, 0x07) AM_READWRITE(vector_8255_2_r, vector_8255_2_w )
	AM_RANGE( 0x0C, 0x0C) AM_WRITE ( vector06_color_set )
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vector06 )

	PORT_START("LINE0")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_DEL)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BkSp") PORT_CODE(KEYCODE_BACKSPACE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Home") PORT_CODE(KEYCODE_HOME)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PgUp") PORT_CODE(KEYCODE_PGUP)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_START("LINE3")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("'") PORT_CODE(KEYCODE_INSERT)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
	PORT_START("LINE4")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_QUOTE)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_START("LINE5")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_START("LINE6")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_START("LINE7")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("~") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
	PORT_START("LINE8")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Rus/Lat") PORT_CODE(KEYCODE_LALT)
	PORT_START("RESET") 
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Reset") PORT_CODE(KEYCODE_F11)		
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Reset 2") PORT_CODE(KEYCODE_F12)		
	
INPUT_PORTS_END

static const cassette_config vector_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED
};


/* Machine driver */
static MACHINE_DRIVER_START( vector06 )
  /* basic machine hardware */
//  	MDRV_CPU_ADD("main", 8080, 3000000)
	MDRV_CPU_ADD("main", Z80, 3000000)
  	MDRV_CPU_PROGRAM_MAP(vector06_mem, 0)
  	MDRV_CPU_IO_MAP(vector06_io, 0)
  	MDRV_CPU_VBLANK_INT("main", vector06_interrupt)

  	MDRV_MACHINE_RESET( vector06 )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( vector06_ppi8255_interface )

	MDRV_DEVICE_ADD( "ppi8255_2", PPI8255 )
	MDRV_DEVICE_CONFIG( vector06_ppi8255_2_interface )

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256+64, 256+64)
	MDRV_SCREEN_VISIBLE_AREA(0, 256+64-1, 0, 256+64-1)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(vector06)

	MDRV_VIDEO_START(vector06)
  	MDRV_VIDEO_UPDATE(vector06)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("cassette", WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_CASSETTE_ADD( "cassette", vector_cassette_config )
  
MACHINE_DRIVER_END

/* ROM definition */

ROM_START( vector06 )
    ROM_REGION( 0x20000, "main", ROMREGION_ERASEFF )
    ROM_SYSTEM_BIOS(0, "unboot32k", "Universal Boot 32K")
    ROMX_LOAD( "unboot32k.rt", 0x10000, 0x8000, CRC(28c9b5cd) SHA1(8cd7fb658896a7066ae93b10eaafa0f12139ad81), ROM_BIOS(1))
    ROM_SYSTEM_BIOS(1, "unboot2k", "Universal Boot 2K")
    ROMX_LOAD( "unboot2k.rt",  0x10000, 0x0800, CRC(4c80dc31) SHA1(7e5e3acfdbea2e52b0d64c5868821deaec383815), ROM_BIOS(2))
    ROM_SYSTEM_BIOS(2, "coman", "Boot Coman")
    ROMX_LOAD( "coman.rt",     0x10000, 0x0800, CRC(f8c4a85a) SHA1(47fa8b02f09a1d06aa63a2b90b2597b1d93d976f), ROM_BIOS(3))
    ROM_SYSTEM_BIOS(3, "bootbyte", "Boot Byte")
    ROMX_LOAD( "bootbyte.rt",  0x10000, 0x0800, CRC(3b42fd9d) SHA1(a112f4fe519bc3dbee85b09040d4804a17c9eda2), ROM_BIOS(4))
    ROM_SYSTEM_BIOS(4, "bootos", "Boot OS")
    ROMX_LOAD( "bootos.rt",    0x10000, 0x0200, CRC(46bef038) SHA1(6732f4a360cd38112c53c458842d31f5b035cf59), ROM_BIOS(5))
    ROM_SYSTEM_BIOS(5, "boot512", "Boot 512")
    ROMX_LOAD( "boot512.rt",   0x10000, 0x0200, CRC(a0b1c6b2) SHA1(f6fe15cb0974aed30f9b7aa72133324a66d1ed3f), ROM_BIOS(6))
    ROM_CART_LOAD(0, "emr", 0x18000, 0x8000, ROM_FILL_FF | ROM_OPTIONAL)
ROM_END

static SYSTEM_CONFIG_START(vector06)
 	CONFIG_RAM_DEFAULT(64 * 1024)
 	CONFIG_DEVICE(cartslot_device_getinfo)
SYSTEM_CONFIG_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT       INIT     CONFIG COMPANY                  FULLNAME   FLAGS */
COMP( 1987, vector06, 	 0,  	 0,	vector06, 	vector06, 	vector06, vector06,  "", 					 "Vector 06c",	 GAME_NOT_WORKING)
