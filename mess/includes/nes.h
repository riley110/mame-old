/***************************************************************************

  nes.h

  Headers for the Nintendo Entertainment System (Famicom).

***************************************************************************/

#ifndef NES_H
#define NES_H

#define BOTTOM_VISIBLE_SCANLINE		239		/* The bottommost visible scanline */
#define NMI_SCANLINE     			244		/* 244 times Bayou Billy perfectly */
#define NTSC_SCANLINES_PER_FRAME	262
#define PAL_SCANLINES_PER_FRAME		305		/* verify - times Elite perfectly */

#define PORT_PAD0		0
#define PORT_PAD1		1
#define PORT_PAD2		2
#define PORT_PAD3		3
#define PORT_ZAPPER0_X	4
#define PORT_ZAPPER0_Y	5
#define PORT_ZAPPER0_T	6
#define PORT_ZAPPER1_X	7
#define PORT_ZAPPER1_Y	8
#define PORT_ZAPPER1_T	9
#define PORT_PADDLE1	10
#define PORT_CONFIG1	11
#define PORT_CONFIG2	12
#define PORT_FLIPKEY	12

void ppu_mirror_custom (int page, int address);
void ppu_mirror_custom_vrom (int page, int address);

extern int ppu_scanlines_per_frame;

WRITE8_HANDLER ( nes_IN0_w );
WRITE8_HANDLER ( nes_IN1_w );

extern int nes_vram_sprite[8];

extern unsigned char *battery_ram;

struct nes_struct
{
	/* load-time cart variables which remain constant */
	UINT8 trainer;
	UINT8 battery;
	UINT8 prg_chunks;
	UINT8 chr_chunks;

	/* system variables which don't change at run-time */
	UINT16 mapper;
	UINT8 four_screen_vram;
	UINT8 hard_mirroring;
	UINT8 slow_banking;

	UINT8 *rom;
	UINT8 *vrom;
	UINT8 *vram;
	UINT8 *wram;

	/* Variables which can change */
	UINT8 mid_ram_enable;
};

extern struct nes_struct nes;

struct fds_struct
{
	UINT8 *data;
	UINT8 sides;

	/* Variables which can change */
	UINT8 motor_on;
	UINT8 door_closed;
	UINT8 current_side;
	UINT32 head_position;
	UINT8 status0;
	UINT8 read_mode;
	UINT8 write_reg;
};

extern struct fds_struct nes_fds;

/* protos */

/* machine/nes.c */
DEVICE_LOAD(nes_cart);
DEVICE_LOAD(nes_disk);
DEVICE_UNLOAD(nes_disk);

MACHINE_INIT( nes );
MACHINE_STOP( nes );

DRIVER_INIT( nes );
DRIVER_INIT( nespal );
 READ8_HANDLER( nes_IN0_r );
 READ8_HANDLER( nes_IN1_r );

void nes_partialhash(char *dest, const unsigned char *data,
	unsigned long length, unsigned int functions);

WRITE8_HANDLER( nes_low_mapper_w );
 READ8_HANDLER ( nes_low_mapper_r );
WRITE8_HANDLER( nes_mid_mapper_w );
 READ8_HANDLER ( nes_mid_mapper_r );
WRITE8_HANDLER( nes_mapper_w );

/* vidhrdw/nes.c */
PALETTE_INIT( nes );
VIDEO_START( nes );
VIDEO_UPDATE( nes );

#endif /* NES_H */


