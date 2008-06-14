/***************************************************************************
    commodore c16 home computer

    PeT mess@utanet.at

    documentation
     www.funet.fi

***************************************************************************/

/*
------------------------------------
c16 commodore c16/c116/c232/c264 (pal version)
plus4   commodore plus4 (ntsc version)
c364 commodore c364/v364 prototype (ntsc version)
------------------------------------
(beta version)

if the game runs to fast with the ntsc version, try the pal version!
flickering affects in one video version, try the other video version

c16(pal version, ntsc version?):
 design like the vc20/c64
 sequel to c64
 other keyboardlayout,
 worse soundchip,
 more colors, but no sprites,
 other tape and gameports plugs
 16 kbyte ram
 newer basic and kernal (ieee floppy support)

c116(pal version, ntsc version?):
 100% software compatible to c16
 small and flat
 gummi keys

232:
 video system versions (?)
 plus4 case
 32 kbyte ram
 userport?, acia6551 chip?

264:
 video system versions (?)
 plus4 case
 64 kbyte ram
 userport?, acia6551 chip?

plus4(pal version, ntsc version?):
 in emu ntsc version!
 case like c116, but with normal keys
 64 kbyte ram
 userport
 build in additional rom with programs

c364 prototype:
 video system versions (?)
 like plus4, but with additional numeric keyblock
 slightly modified kernel rom
 build in speech hardware/rom

state
-----
rasterline based video system
 imperfect scrolling support (when 40 columns or 25 lines)
 lightpen support missing?
 should be enough for 95% of the games and programs
imperfect sound
keyboard, joystick 1 and 2
no speech hardware (c364)
simple tape support
serial bus
 simple disk drives
 no printer or other devices
expansion modules
 rom cartridges
 simple ieee488 floppy support (c1551 floppy disk drive)
 no other expansion modules
no userport (plus4)
 no rs232/v.24 interface
quickloader

some unsolved problems
 memory check by c16 kernel will not recognize more memory without restart of mess
 cpu clock switching/changing (overclocking should it be)

Keys
----
Some PC-Keyboards does not behave well when special two or more keys are
pressed at the same time
(with my keyboard printscreen clears the pressed pause key!)

shift-cbm switches between upper-only and normal character set
(when wrong characters on screen this can help)
run (shift-stop) loads first program from device 8 (dload"*) and starts it
stop-reset activates monitor (use x to leave it)

Tape
----
(DAC 1 volume in noise volume)
loading of wav, prg and prg files in zip archiv
commandline -cassette image
wav:
 8 or 16(not tested) bit, mono, 5000 Hz minimum
 has the same problems like an original tape drive (tone head must
 be adjusted to get working(no load error,...) wav-files)
zip:
 must be placed in current directory
 prg's are played in the order of the files in zip file

use LOAD or LOAD"" or LOAD"",1 for loading of normal programs
use LOAD"",1,1 for loading programs to their special address

several programs relies on more features
(loading other file types, writing, ...)

Discs
-----
only file load from drive 8 and 9 implemented
 loads file from rom directory (*.prg,*p00) (must NOT be specified on commandline)
 or file from d64 image (here also directory LOAD"$",8 supported)
use DLOAD"filename"
or LOAD"filename",8
or LOAD"filename",8,1 (for loading machine language programs at their address)
for loading
type RUN or the appropriate sys call to start them

several programs rely on more features
(loading other file types, writing, ...)

most games rely on starting own programs in the floppy drive
(and therefore cpu level emulation is needed)

Roms
----
.bin .rom .lo .hi .prg
files with boot-sign in it
  recogniced as roms

.prg files loaded at address in its first two bytes
.bin, .rom, .lo , .hi roms loaded to cs1 low, cs1 high, cs2 low, cs2 high
 address accordingly to order in command line

Quickloader
-----------
.prg files supported
loads program into memory and sets program end pointer
(works with most programs)
program ready to get started with RUN
loads first rom when you press quickload key (f8)

when problems start with -log and look into error.log file
 */


#include "driver.h"
#include "sound/sid6581.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "includes/c16.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"
#include "machine/cbmipt.h"
#include "video/ted7360.h"
#include "devices/cartslot.h"

/*
 * commodore c16/c116/plus 4
 * 16 KByte (C16/C116) or 32 KByte or 64 KByte (plus4) RAM
 * 32 KByte Rom (C16/C116) 64 KByte Rom (plus4)
 * availability to append additional 64 KByte Rom
 *
 * ports 0xfd00 till 0xff3f are always read/writeable for the cpu
 * for the video interface chip it seams to read from
 * ram or from rom in this  area
 *
 * writes go always to ram
 * only 16 KByte Ram mapped to 0x4000,0x8000,0xc000
 * only 32 KByte Ram mapped to 0x8000
 *
 * rom bank at 0x8000: 16K Byte(low bank)
 * first: basic
 * second(plus 4 only): plus4 rom low
 * third: expansion slot
 * fourth: expansion slot
 * rom bank at 0xc000: 16K Byte(high bank)
 * first: kernal
 * second(plus 4 only): plus4 rom high
 * third: expansion slot
 * fourth: expansion slot
 * writes to 0xfddx select rom banks:
 * address line 0 and 1: rom bank low
 * address line 2 and 3: rom bank high
 *
 * writes to 0xff3e switches to roms (0x8000 till 0xfd00, 0xff40 till 0xffff)
 * writes to 0xff3f switches to rams
 *
 * at 0xfc00 till 0xfcff is ram or rom kernal readable
 */

static ADDRESS_MAP_START( c16_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READ( SMH_BANK9)
	AM_RANGE(0x4000, 0x7fff) AM_READ( SMH_BANK1)	   /* only ram memory configuration */
	AM_RANGE(0x8000, 0xbfff) AM_READ( SMH_BANK2)
	AM_RANGE(0xc000, 0xfbff) AM_READ( SMH_BANK3)
	AM_RANGE(0xfc00, 0xfcff) AM_READ( SMH_BANK4)
	AM_RANGE(0xfd10, 0xfd1f) AM_READ( c16_fd1x_r)
	AM_RANGE(0xfd30, 0xfd3f) AM_READ( c16_6529_port_r) /* 6529 keyboard matrix */
#if 0
	AM_RANGE( 0xfd40, 0xfd5f) AM_READ( sid6581_0_port_r ) /* sidcard, eoroidpro ... */
	AM_RANGE(0xfec0, 0xfedf) AM_READ( c16_iec9_port_r) /* configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_READ( c16_iec8_port_r) /* configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_READ( ted7360_port_r)
	AM_RANGE(0xff20, 0xffff) AM_READ( SMH_BANK8)
/*  { 0x10000, 0x3ffff, SMH_ROM }, */
ADDRESS_MAP_END

static ADDRESS_MAP_START( c16_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_WRITE( SMH_BANK9)
	AM_RANGE(0x4000, 0x7fff) AM_WRITE( SMH_BANK5)
	AM_RANGE(0x8000, 0xbfff) AM_WRITE( SMH_BANK6)
	AM_RANGE(0xc000, 0xfcff) AM_WRITE( SMH_BANK7)
#if 0
	AM_RANGE(0x4000, 0x7fff) AM_WRITE( c16_write_4000)  /*configured in c16_common_init */
	AM_RANGE(0x8000, 0xbfff) AM_WRITE( c16_write_8000)  /*configured in c16_common_init */
	AM_RANGE(0xc000, 0xfcff) AM_WRITE( c16_write_c000)  /*configured in c16_common_init */
#endif
	AM_RANGE(0xfd30, 0xfd3f) AM_WRITE( c16_6529_port_w) /* 6529 keyboard matrix */
#if 0
	AM_RANGE(0xfd40, 0xfd5f) AM_WRITE( sid6581_0_port_w)
#endif
	AM_RANGE(0xfdd0, 0xfddf) AM_WRITE( c16_select_roms) /* rom chips selection */
#if 0
	AM_RANGE(0xfec0, 0xfedf) AM_WRITE( c16_iec9_port_w) /*configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_WRITE( c16_iec8_port_w) /*configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_WRITE( ted7360_port_w)
#if 0
	AM_RANGE(0xff20, 0xff3d) AM_WRITE( c16_write_ff20)  /*configure in c16_common_init */
#endif
	AM_RANGE(0xff3e, 0xff3e) AM_WRITE( c16_switch_to_rom)
	AM_RANGE(0xff3f, 0xff3f) AM_WRITE( c16_switch_to_ram)
#if 0
	AM_RANGE(0xff40, 0xffff) AM_WRITE( c16_write_ff40)  /*configure in c16_common_init */
//  {0x10000, 0x3ffff, SMH_ROM},
#endif
ADDRESS_MAP_END

static ADDRESS_MAP_START( plus4_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_READ( SMH_BANK9)
	AM_RANGE(0x8000, 0xbfff) AM_READ( SMH_BANK2)
	AM_RANGE(0xc000, 0xfbff) AM_READ( SMH_BANK3)
	AM_RANGE(0xfc00, 0xfcff) AM_READ( SMH_BANK4)
	AM_RANGE(0xfd00, 0xfd0f) AM_READ( c16_6551_port_r)
	AM_RANGE(0xfd10, 0xfd1f) AM_READ( plus4_6529_port_r)
	AM_RANGE(0xfd30, 0xfd3f) AM_READ( c16_6529_port_r) /* 6529 keyboard matrix */
#if 0
	AM_RANGE( 0xfd40, 0xfd5f) AM_READ( sid6581_0_port_r ) /* sidcard, eoroidpro ... */
	AM_RANGE(0xfec0, 0xfedf) AM_READ( c16_iec9_port_r) /* configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_READ( c16_iec8_port_r) /* configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_READ( ted7360_port_r)
	AM_RANGE(0xff20, 0xffff) AM_READ( SMH_BANK8)
/*  { 0x10000, 0x3ffff, SMH_ROM }, */
ADDRESS_MAP_END

static ADDRESS_MAP_START( plus4_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0xfcff) AM_WRITE( SMH_BANK9)
	AM_RANGE(0xfd00, 0xfd0f) AM_WRITE( c16_6551_port_w)
	AM_RANGE(0xfd10, 0xfd1f) AM_WRITE( plus4_6529_port_w)
	AM_RANGE(0xfd30, 0xfd3f) AM_WRITE( c16_6529_port_w) /* 6529 keyboard matrix */
#if 0
	AM_RANGE(0xfd40, 0xfd5f) AM_WRITE( sid6581_0_port_w)
#endif
	AM_RANGE(0xfdd0, 0xfddf) AM_WRITE( c16_select_roms) /* rom chips selection */
#if 0
	AM_RANGE(0xfec0, 0xfedf) AM_WRITE( c16_iec9_port_w) /*configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_WRITE( c16_iec8_port_w) /*configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_WRITE( ted7360_port_w)
	AM_RANGE(0xff20, 0xff3d) AM_WRITE( SMH_RAM)
	AM_RANGE(0xff3e, 0xff3e) AM_WRITE( c16_switch_to_rom)
	AM_RANGE(0xff3f, 0xff3f) AM_WRITE( c16_switch_to_ram)
	AM_RANGE(0xff40, 0xffff) AM_WRITE( SMH_RAM)
//  {0x10000, 0x3ffff, SMH_ROM},
ADDRESS_MAP_END

static ADDRESS_MAP_START( c364_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_READ( SMH_BANK9)
	AM_RANGE(0x8000, 0xbfff) AM_READ( SMH_BANK2)
	AM_RANGE(0xc000, 0xfbff) AM_READ( SMH_BANK3)
	AM_RANGE(0xfc00, 0xfcff) AM_READ( SMH_BANK4)
	AM_RANGE(0xfd00, 0xfd0f) AM_READ( c16_6551_port_r)
	AM_RANGE(0xfd10, 0xfd1f) AM_READ( plus4_6529_port_r)
	AM_RANGE(0xfd20, 0xfd2f) AM_READ( c364_speech_r )
	AM_RANGE(0xfd30, 0xfd3f) AM_READ( c16_6529_port_r) /* 6529 keyboard matrix */
#if 0
	AM_RANGE( 0xfd40, 0xfd5f) AM_READ( sid6581_0_port_r ) /* sidcard, eoroidpro ... */
	AM_RANGE(0xfec0, 0xfedf) AM_READ( c16_iec9_port_r) /* configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_READ( c16_iec8_port_r) /* configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_READ( ted7360_port_r)
	AM_RANGE(0xff20, 0xffff) AM_READ( SMH_BANK8)
/*  { 0x10000, 0x3ffff, SMH_ROM }, */
ADDRESS_MAP_END

static ADDRESS_MAP_START( c364_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0xfcff) AM_WRITE( SMH_BANK9)
	AM_RANGE(0xfd00, 0xfd0f) AM_WRITE( c16_6551_port_w)
	AM_RANGE(0xfd10, 0xfd1f) AM_WRITE( plus4_6529_port_w)
	AM_RANGE(0xfd20, 0xfd2f) AM_WRITE( c364_speech_w )
	AM_RANGE(0xfd30, 0xfd3f) AM_WRITE( c16_6529_port_w) /* 6529 keyboard matrix */
#if 0
	AM_RANGE(0xfd40, 0xfd5f) AM_WRITE( sid6581_0_port_w)
#endif
	AM_RANGE(0xfdd0, 0xfddf) AM_WRITE( c16_select_roms) /* rom chips selection */
#if 0
	AM_RANGE(0xfec0, 0xfedf) AM_WRITE( c16_iec9_port_w) /*configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_WRITE( c16_iec8_port_w) /*configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_WRITE( ted7360_port_w)
	AM_RANGE(0xff20, 0xff3d) AM_WRITE( SMH_RAM)
	AM_RANGE(0xff3e, 0xff3e) AM_WRITE( c16_switch_to_rom)
	AM_RANGE(0xff3f, 0xff3f) AM_WRITE( c16_switch_to_ram)
	AM_RANGE(0xff40, 0xffff) AM_WRITE( SMH_RAM)
//  {0x10000, 0x3ffff, SMH_ROM},
ADDRESS_MAP_END


/*************************************
 *
 *  Input Ports
 *
 *************************************/

static INPUT_PORTS_START( c16 )
	PORT_INCLUDE( common_cbm_keyboard )		/* ROW0 -> ROW7 */

	PORT_MODIFY("ROW0")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("@") PORT_CODE(KEYCODE_OPENBRACE)				PORT_CHAR('@')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F3)									PORT_CHAR(UCHAR_MAMEKEY(F3)) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2)									PORT_CHAR(UCHAR_MAMEKEY(F2)) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1)									PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("HELP f7") PORT_CODE(KEYCODE_F4)				PORT_CHAR(UCHAR_MAMEKEY(F8)) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_INSERT)								PORT_CHAR(0xA3)
	PORT_MODIFY("ROW4")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("0  \xE2\x86\x91") PORT_CODE(KEYCODE_0)		PORT_CHAR('0') PORT_CHAR(0x2191)
	PORT_MODIFY("ROW5")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)					PORT_CHAR('-')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2)							PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_PGUP)									PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_MODIFY("ROW6")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)							PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("=  Pi  \xE2\x86\x90") PORT_CODE(KEYCODE_PGDN)	PORT_CHAR('=') PORT_CHAR(0x03C0) PORT_CHAR(0x2190)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ESC)									PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)								PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH)								PORT_CHAR('*')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)									PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_MODIFY("ROW7")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Home Clear") PORT_CODE(KEYCODE_DEL)			PORT_CHAR(UCHAR_MAMEKEY(HOME))

	PORT_INCLUDE( c16_special )				/* SPECIAL */

	PORT_INCLUDE( c16_controls )			/* JOY0, JOY1 */

	/* no real floppy */

	PORT_INCLUDE( c16_config )				/* DSW0, CFG0, CFG1 */
INPUT_PORTS_END


static INPUT_PORTS_START( c16c )
	PORT_INCLUDE( c16 )

	/* c1551 floppy */

	PORT_MODIFY("CFG0")
	PORT_BIT( 0x38, 0x10, IPT_UNUSED )
	PORT_BIT( 0x07, 0x00, IPT_UNUSED )
INPUT_PORTS_END


static INPUT_PORTS_START( c16v )
	PORT_INCLUDE( c16 )
	
	/* vc1541 floppy */
	
	PORT_MODIFY("CFG0")
	PORT_BIT( 0x38, 0x10, IPT_UNUSED )
	PORT_BIT( 0x07, 0x00, IPT_UNUSED )
INPUT_PORTS_END


static INPUT_PORTS_START( plus4 )
	PORT_INCLUDE( c16 )

	/* no real floppy */
	
	PORT_MODIFY( "ROW0" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)					PORT_CHAR(0xA3)
	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)						PORT_CHAR('-')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_UP)							PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DOWN)							PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)							PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("=  Pi  \xE2\x86\x90") PORT_CODE(KEYCODE_BACKSLASH2)	PORT_CHAR('=') PORT_CHAR(0x03C0) PORT_CHAR(0x2190)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RIGHT)							PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_INSERT)						PORT_CHAR('*')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LEFT)							PORT_CHAR(UCHAR_MAMEKEY(LEFT))

	PORT_MODIFY("CFG1")
	PORT_BIT( 0x10, 0x10, IPT_UNUSED )			/* ntsc */
	PORT_BIT( 0x0c, 0x04, IPT_UNUSED )			/* plus4 */
INPUT_PORTS_END


static INPUT_PORTS_START (plus4c)
	PORT_INCLUDE( plus4 )

	/* c1551 floppy */
	
	PORT_MODIFY("CFG0")
	PORT_BIT( 0x38, 0x10, IPT_UNUSED )
	PORT_BIT( 0x07, 0x00, IPT_UNUSED )
	PORT_MODIFY("CFG1")
	PORT_BIT( 0x10, 0x10, IPT_UNUSED )			/* ntsc */
	PORT_BIT( 0x0c, 0x04, IPT_UNUSED )			/* plus4 */
INPUT_PORTS_END


static INPUT_PORTS_START (plus4v)
	PORT_INCLUDE( plus4 )

	/* vc1541 floppy */
	
	PORT_MODIFY("CFG0")
	PORT_BIT( 0x38, 0x20, IPT_UNUSED )
	PORT_BIT( 0x07, 0x00, IPT_UNUSED )
	PORT_MODIFY("CFG1")
	PORT_BIT( 0x10, 0x10, IPT_UNUSED )			/* ntsc */
	PORT_BIT( 0x0c, 0x04, IPT_UNUSED )			/* plus4 */
INPUT_PORTS_END


#if 0
static INPUT_PORTS_START (c364)
	PORT_INCLUDE( plus4 )

	/* no real floppy */
	
	PORT_MODIFY("CFG1")
	PORT_BIT( 0x10, 0x10, IPT_UNUSED )			/* ntsc */
	PORT_BIT( 0x0c, 0x08, IPT_UNUSED )			/* 364 */
	/* numeric block - hardware wired to other keys? */
	/* check cbmipt.c for layout */
INPUT_PORTS_END
#endif



/* Initialise the c16 palette */
static PALETTE_INIT( c16 )
{
	int i;

	for ( i = 0; i < sizeof(ted7360_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i, ted7360_palette[i*3], ted7360_palette[i*3+1], ted7360_palette[i*3+2]);
	}
}

#if 0
/* cbm version in kernel at 0xff80 (offset 0x3f80)
   0x80 means pal version */

	 /* basic */
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))

	 /* kernal pal */
	 ROM_LOAD("318004.05",    0x14000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb))
	 ROM_LOAD ("318004.03", 0x14000, 0x4000, CRC(77bab934))

	 /* kernal ntsc */
	 ROM_LOAD ("318005.05", 0x14000, 0x4000, CRC(70295038) SHA1(a3d9e5be091b98de39a046ab167fb7632d053682))
	 ROM_LOAD ("318005.04", 0x14000, 0x4000, CRC(799a633d))

	 /* 3plus1 program */
	 ROM_LOAD ("317053.01", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f))
	 ROM_LOAD ("317054.01", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693))

	 /* same as 109de2fc, but saved from running machine, so
        io area different ! */
	 ROM_LOAD ("3plus1hi.rom", 0x1c000, 0x4000, CRC(aab61387))
	 /* same but lo and hi in one rom */
	 ROM_LOAD ("3plus1.rom", 0x18000, 0x8000, CRC(7d464449))
#endif

ROM_START (c16)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD("318004.05",    0x14000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb))
ROM_END

ROM_START (c16hun)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD("hungary.bin",    0x14000, 0x4000, CRC(775f60c5) SHA1(20cf3c4bf6c54ef09799af41887218933f2e27ee))
ROM_END

ROM_START (c16c)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD("318004.05",    0x14000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb))
	 C1551_ROM (REGION_CPU2)
ROM_END

ROM_START (c16v)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD("318004.05",    0x14000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb))
	 VC1541_ROM (REGION_CPU2)
ROM_END

ROM_START (plus4)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD ("318005.05", 0x14000, 0x4000, CRC(70295038) SHA1(a3d9e5be091b98de39a046ab167fb7632d053682))
	 ROM_LOAD ("317053.01", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f))
	 ROM_LOAD ("317054.01", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693))
ROM_END

ROM_START (plus4c)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD ("318005.05", 0x14000, 0x4000, CRC(70295038) SHA1(a3d9e5be091b98de39a046ab167fb7632d053682))
	 ROM_LOAD ("317053.01", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f))
	 ROM_LOAD ("317054.01", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693))
	 C1551_ROM (REGION_CPU2)
ROM_END

ROM_START (plus4v)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD ("318005.05", 0x14000, 0x4000, CRC(70295038) SHA1(a3d9e5be091b98de39a046ab167fb7632d053682))
	 ROM_LOAD ("317053.01", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f))
	 ROM_LOAD ("317054.01", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693))
	 VC1541_ROM (REGION_CPU2)
ROM_END

ROM_START (c364)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD ("kern364p.bin", 0x14000, 0x4000, CRC(84fd4f7a) SHA1(b9a5b5dacd57ca117ef0b3af29e91998bf4d7e5f))
	 ROM_LOAD ("317053.01", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f))
	 ROM_LOAD ("317054.01", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693))
	 /* at address 0x20000 not so good */
	 ROM_LOAD ("spk3cc4.bin", 0x28000, 0x4000, CRC(5227c2ee) SHA1(59af401cbb2194f689898271c6e8aafa28a7af11))
ROM_END



static MACHINE_DRIVER_START( c16 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M7501, 1400000)        /* 7.8336 Mhz */
	MDRV_CPU_PROGRAM_MAP(c16_readmem, c16_writemem)
	MDRV_CPU_VBLANK_INT("main", c16_frame_interrupt)
	MDRV_CPU_PERIODIC_INT(ted7360_raster_interrupt, TED7360_HRETRACERATE)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( c16 )

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(TED7360PAL_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(336, 216)
	MDRV_SCREEN_VISIBLE_AREA(0, 336 - 1, 0, 216 - 1)
	MDRV_PALETTE_LENGTH(sizeof (ted7360_palette) / sizeof (ted7360_palette[0]) / 3)
	MDRV_PALETTE_INIT(c16)

	MDRV_VIDEO_START( ted7360 )
	MDRV_VIDEO_UPDATE( ted7360 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("ted7360", CUSTOM, 0)
	MDRV_SOUND_CONFIG(ted7360_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD_TAG("sid", SID8580, TED7360PAL_CLOCK/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_QUICKLOAD_ADD(cbm_c16, "p00,prg", CBM_QUICKLOAD_DELAY_SECONDS)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c16c )
	MDRV_IMPORT_FROM( c16 )
	MDRV_IMPORT_FROM( cpu_c1551 )
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(100)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c16v )
	MDRV_IMPORT_FROM( c16 )
	MDRV_IMPORT_FROM( cpu_vc1541 )
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(5000)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( plus4 )
	MDRV_IMPORT_FROM( c16 )
	MDRV_CPU_REPLACE( "main", M7501, 1200000)
	MDRV_CPU_PROGRAM_MAP( plus4_readmem, plus4_writemem )
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(TED7360NTSC_VRETRACERATE)

	MDRV_SOUND_REPLACE("sid", SID8580, TED7360NTSC_CLOCK/4)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( plus4c )
	MDRV_IMPORT_FROM( plus4 )
	MDRV_IMPORT_FROM( cpu_c1551 )
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(1000)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( plus4v )
	MDRV_IMPORT_FROM( plus4 )
	MDRV_IMPORT_FROM( cpu_vc1541 )

	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(5000)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c364 )
	MDRV_IMPORT_FROM( plus4 )
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(c364_readmem, c364_writemem)
MACHINE_DRIVER_END

static DRIVER_INIT( c16 )		{ c16_driver_init(machine); }
#ifdef UNUSED_FUNCTION
DRIVER_INIT( c16hun )	{ c16_driver_init(machine); }
DRIVER_INIT( c16c )		{ c16_driver_init(machine); }
DRIVER_INIT( c16v )		{ c16_driver_init(machine); }
#endif
static DRIVER_INIT( plus4 )	{ c16_driver_init(machine); }
#ifdef UNUSED_FUNCTION
DRIVER_INIT( plus4c )	{ c16_driver_init(machine); }
DRIVER_INIT( plus4v )	{ c16_driver_init(machine); }
DRIVER_INIT( c364 )		{ c16_driver_init(machine); }
#endif

static void c16cart_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(c16_rom); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "bin,rom"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(c16)
	CONFIG_DEVICE(c16cart_device_getinfo)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_RAM(16 * 1024)
	CONFIG_RAM(32 * 1024)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(c16c)
	CONFIG_DEVICE(c16cart_device_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_DEVICE(c1551_device_getinfo)
	CONFIG_RAM(16 * 1024)
	CONFIG_RAM(32 * 1024)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(c16v)
	CONFIG_DEVICE(c16cart_device_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_DEVICE(vc1541_device_getinfo)
	CONFIG_RAM(16 * 1024)
	CONFIG_RAM(32 * 1024)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(plus)
	CONFIG_DEVICE(c16cart_device_getinfo)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(plusc)
	CONFIG_DEVICE(c16cart_device_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_DEVICE(c1551_device_getinfo)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(plusv)
	CONFIG_DEVICE(c16cart_device_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_DEVICE(vc1541_device_getinfo)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

/*      YEAR    NAME    PARENT  COMPAT  MACHINE INPUT   INIT    CONFIG   COMPANY                                FULLNAME */
COMP ( 1984,	c16,	0,		0,		c16,	c16,	c16,	c16,     "Commodore Business Machines Co.",      "Commodore 16/116/232/264 (PAL)", 0)
COMP ( 1984,	c16hun, c16,	0,		c16,	c16,	c16,	c16,     "Commodore Business Machines Co.",      "Commodore 16 Novotrade (PAL, Hungarian Character Set)", 0)
COMP ( 1984,	c16c,	c16,	0,		c16c,	c16c,	c16,	c16c,    "Commodore Business Machines Co.",      "Commodore 16/116/232/264 (PAL), 1551", 0 )
COMP ( 1984,	plus4,	c16,	0,		plus4,	plus4,	plus4,	plus,    "Commodore Business Machines Co.",      "Commodore +4 (NTSC)", 0)
COMP ( 1984,	plus4c, c16,	0,		plus4c, plus4c, plus4,	plusc,   "Commodore Business Machines Co.",      "Commodore +4 (NTSC), 1551", 0 )
COMP ( 1984,	c364,	c16,	0,		c364,	plus4,	plus4,	plusv,   "Commodore Business Machines Co.",      "Commodore 364 (Prototype)", GAME_IMPERFECT_SOUND)
COMP ( 1984,	c16v,	c16,	0,		c16v,	c16v,	c16,	c16v,    "Commodore Business Machines Co.",      "Commodore 16/116/232/264 (PAL), VC1541", GAME_NOT_WORKING)
COMP ( 1984,	plus4v, c16,	0,		plus4v, plus4v, plus4,	plusv,   "Commodore Business Machines Co.",      "Commodore +4 (NTSC), VC1541", GAME_NOT_WORKING)
