/***************************************************************************

        Sun-1

        Documentation:
            http://www.bitsavers.org/pdf/sun/sun1/800-0345_Sun-1_System_Ref_Man_Jul82.pdf
            (page 39,40 of pdf contain memory map)

        04/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/upd7201.h"
#include "machine/terminal.h"

static UINT16* sun1_ram;

// Just hack to show output since upd7201 is not implemented yet

static READ16_HANDLER(sun1_upd7201_r)
{
	return 0xffff;
}

static WRITE16_HANDLER(sun1_upd7201_w)
{
	running_device *devconf = space->machine->device("terminal");
	if (offset==0) terminal_write(devconf,0,data >> 8);
}

static ADDRESS_MAP_START(sun1_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM AM_BASE(&sun1_ram) // 512 KB RAM / ROM at boot
	AM_RANGE(0x00200000, 0x00203fff) AM_ROM AM_REGION("user1",0)
	AM_RANGE(0x00600000, 0x00600007) AM_READWRITE( sun1_upd7201_r, sun1_upd7201_w )
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sun1 )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(sun1)
{
	UINT8* user1 = memory_region(machine, "user1");

	memcpy((UINT8*)sun1_ram,user1,0x4000);

	machine->device("maincpu")->reset();
}


static WRITE8_DEVICE_HANDLER( sun1_kbd_put )
{
}

static GENERIC_TERMINAL_INTERFACE( sun1_terminal_intf )
{
	DEVCB_HANDLER(sun1_kbd_put)
};


static MACHINE_CONFIG_START( sun1, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", M68000, XTAL_10MHz)
    MDRV_CPU_PROGRAM_MAP(sun1_mem)

    MDRV_MACHINE_RESET(sun1)

    /* video hardware */
    MDRV_FRAGMENT_ADD( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,sun1_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sun1 )
    ROM_REGION( 0x4000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "v10.8.bin", 0x0001, 0x2000, CRC(3528a0f8) SHA1(be437dd93d1a44eccffa6f5e05935119482beab0))
	ROM_LOAD16_BYTE( "v10.0.bin", 0x0000, 0x2000, CRC(1ad4c52a) SHA1(4bc1a19e8f202378d5d7baa8b95319275c040a6d))
	ROM_REGION( 0x4000, "diag", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "8mhzdiag.8.bin", 0x0001, 0x2000, CRC(808a549e) SHA1(d2aba014a5507c1538f2c1a73e1d2524f28034f4))
	ROM_LOAD16_BYTE( "8mhzdiag.0.bin", 0x0000, 0x2000, CRC(7a92d506) SHA1(5df3800f7083293fc01bb6a7e7538ad425bbebfb))
	ROM_REGION( 0x10000, "gfx", ROMREGION_ERASEFF )
	ROM_LOAD( "gfxu605.g4.bin",  0x0000, 0x0200, CRC(274b7b3d) SHA1(40d8be2cfcbd03512a05925991bb5030d5d4b5e9))
	ROM_LOAD( "gfxu308.g21.bin", 0x0200, 0x0200, CRC(35a6eed8) SHA1(25cb2dd8e5343cd7927c3045eb4cb96dc9935a37))
	ROM_LOAD( "gfxu108.g20.bin", 0x0400, 0x0200, CRC(ecee335e) SHA1(5f4d32dc918af15872cd6e700a04720caeb6c657))
	ROM_LOAD( "gfxu105.g0.bin",  0x0600, 0x0200, CRC(8e1a24b3) SHA1(dad2821c3a3137ad69e78b6fc29ab582e5d78646))
	ROM_LOAD( "gfxu104.g1.bin",  0x0800, 0x0200, CRC(86f7a483) SHA1(8eb3778f5497741cd4345e81ff1a903c9a63c8bb))
	ROM_LOAD( "gfxu307.g61.bin", 0x0a00, 0x0020, CRC(b190f25d) SHA1(80fbdc843f1eb68a2d3713499f04d99dab88ce83))
	ROM_LOAD( "gfxu107.g60.bin", 0x0a20, 0x0020, CRC(425d3a98) SHA1(9ae4ce3761c2f995d00bed8d752c55224d274062))
	ROM_REGION( 0x10000, "cpu", ROMREGION_ERASEFF )
	ROM_LOAD( "cpuu503.p2.bin", 0x0000, 0x0200, CRC(12d9a6be) SHA1(fca99f9c5afc630ac67cbd4e5ba4e5242b826848))
	ROM_LOAD( "cpuu602.p1.bin", 0x0200, 0x0020, CRC(ee1e5a14) SHA1(0d3346cb3b647fa2475bd7b4fa36ea6ecfdaf805))
	ROM_LOAD( "cpuu502.p0.bin", 0x0220, 0x0020, CRC(20eb1183) SHA1(9b268792b28d858d6b6a1b6c4148af88a8d6b735))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1982, sun1,  0,       0,		 sun1,		sun1,	 0, 	 "Sun Microsystems",   "Sun-1",		GAME_NOT_WORKING | GAME_NO_SOUND)

