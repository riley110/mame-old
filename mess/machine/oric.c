/* ORIC driver */

/* By:


  - Paul Cook
  - Kev Thacker

  Thankyou to Fabrice Frances for his ORIC documentation which helped with this driver
  http://oric.ifrance.com/oric/

  TODO:
   - there are problems loading some .wav's. Try to fix these.
   - fix more graphics display problems
   - check the printer works
   - fix more disc drive/wd179x problems so more software will run
*/

#include <stdio.h>
#include "driver.h"
#include "includes/oric.h"
#include "includes/basicdsk.h"
#include "includes/wd179x.h"
#include "machine/6522via.h"
#include "includes/mfmdisk.h"
#include "includes/6551.h"
#include "includes/centroni.h"
#include "printer.h"

/* timer used to refresh via cb input, which will trigger ints on pulses
from tape */
static void *oric_tape_timer;
/* ==0 if oric1 or oric atmos, !=0 if telestrat */
static int oric_is_telestrat;

/* This does not exist in the real hardware. I have used it to
know which sources are interrupting */
/* bit 2 = telestrat 2nd via interrupt, 1 = microdisc interface,
0 = oric 1st via interrupt */
static unsigned char oric_irqs;
enum
{
	ORIC_FLOPPY_NONE,
	ORIC_FLOPPY_MFM_DISK,
	ORIC_FLOPPY_BASIC_DISK
};

/* called when ints are changed - cleared/set */
static void oric_refresh_ints(void)
{
	if (oric_is_telestrat==0)
	{
		/* if microdisc interface is disabled, do not allow interrupts from it */
		if ((readinputport(9) & 0x01)==0)
		{
			oric_irqs &=~(1<<1);
		}
	}

	/* any irq set? */
	if ((oric_irqs & 0x07)!=0)
	{
		cpu_set_irq_line(0,0, HOLD_LINE);
	}
	else
	{
		cpu_set_irq_line(0,0, CLEAR_LINE);
	}
}

static int oric_floppy_type[4] = {ORIC_FLOPPY_NONE, ORIC_FLOPPY_NONE, ORIC_FLOPPY_NONE, ORIC_FLOPPY_NONE};

static	char *oric_ram_0x0c000;


/* index of keyboard line to scan */
static char oric_keyboard_line;
/* sense result */
static char oric_key_sense_bit;
/* mask to read keys */
static char oric_keyboard_mask;


static unsigned char oric_via_port_a_data;

#define ORIC_DUMP_RAM

#ifdef ORIC_DUMP_RAM
/* load image */
void oric_dump_ram(void)
{
	void *file;

	file = osd_fopen(Machine->gamedrv->name, "oricram.bin", OSD_FILETYPE_MEMCARD,OSD_FOPEN_WRITE);
 
	if (file)
	{
		int i;
		for (i=0; i<65536; i++)
		{
			char data;

			data = cpu_readmem16(i);

			osd_fwrite(file, &data, 1);

		}

		/* close file */
		osd_fclose(file);
	}
}
#endif
/* refresh keyboard sense */
static void oric_keyboard_sense_refresh(void)
{
	/* The following assumes that if a 0 is written, it can be used to detect if any key
	has been pressed.. */
	/* for each bit that is 0, it combines it's pressed state with the pressed state so far */

	int i;
	unsigned char key_bit = 0;

	/* what if data is 0, can it sense if any of the keys on a line are pressed? */
	int input_port_data = readinputport(1+oric_keyboard_line);

	/* go through all bits in line */
	for (i=0; i<8; i++)
	{
		/* sense this bit? */
		if (((~oric_keyboard_mask) & (1<<i))!=0)
		{
			/* is key pressed? */
			if (input_port_data & (1<<i))
			{
				/* yes */
				key_bit |= 1;
			}
		}
	}

	/* clear sense result */
	oric_key_sense_bit = 0;
	/* any keys pressed on this line? */
	if (key_bit!=0)
	{
		/* set sense result */
		oric_key_sense_bit = (1<<3);
	}
}


/* this is executed when a write to psg port a is done */
WRITE_HANDLER (oric_psg_porta_write)
{
	oric_keyboard_mask = data;
}


/* PSG control pins */
/* bit 1 = BDIR state */
/* bit 0 = BC1 state */
static char oric_psg_control;

/* this port is also used to read printer data */
READ_HANDLER ( oric_via_in_a_func )
{
	logerror("port a read\r\n");

	/* access psg? */
	if (oric_psg_control!=0)
	{
		/* if psg is in read register state return reg data */
		if (oric_psg_control==0x01)
		{
			return AY8910_read_port_0_r(0);
		}

		/* return high-impedance */
		return 0x0ff;
	}

	/* correct?? */
	return oric_via_port_a_data;
}

READ_HANDLER ( oric_via_in_b_func )
{
	int data;

	oric_keyboard_sense_refresh();

	data = oric_key_sense_bit;
	data |= oric_keyboard_line & 0x07;

	return data;
}


/* read/write data depending on state of bdir, bc1 pins and data output to psg */
static void oric_psg_connection_refresh(void)
{
	if (oric_psg_control!=0)
	{
		switch (oric_psg_control)
		{
			/* write register data */
			case 2:
			{
				AY8910_write_port_0_w (0, oric_via_port_a_data);
			}
			break;
			/* write register index */
			case 3:
			{
				AY8910_control_port_0_w (0, oric_via_port_a_data);
			}
			break;

			default:
				break;
		}

		return;
	}
}

WRITE_HANDLER ( oric_via_out_a_func )
{
	oric_via_port_a_data = data;

	oric_psg_connection_refresh();


	if (oric_psg_control==0)
	{	
		/* if psg not selected, write to printer */
		centronics_write_data(0,data);
	}
}

/*
PB0..PB2
 keyboard lines-demultiplexer

PB3
 keyboard sense line

PB4
 printer strobe line

PB5
 (not connected)

PB6
 tape connector motor control

PB7
 tape connector output

 */

#ifdef ORIC_DUMP_RAM
static int previous_input_port5;
#endif

/* not called yet - this will update the via with the state of the tape data.
This allows the via to trigger on bit changes and issue interrupts */
static void    oric_refresh_tape(int dummy)
{
	int data;

	data = 0;

	if (device_input(IO_CASSETTE,0) > 255)
		data |=1;

    via_set_input_cb1(0, data);

#ifdef ORIC_DUMP_RAM
	{
		int input_port_data = readinputport(5);

		if (((previous_input_port5^input_port_data) & 0x01)!=0)
		{
			if (input_port_data & 0x01) 
			{
				logerror("do dump");
				oric_dump_ram();
			}
		}
		previous_input_port5 = input_port_data;
	}
#endif
}

static unsigned char previous_portb_data = 0;

WRITE_HANDLER ( oric_via_out_b_func )
{
	int printer_handshake;

	/* KEYBOARD */
	oric_keyboard_line = data & 0x07;

	/* CASSETTE */
	/* cassette motor control */
	device_status(IO_CASSETTE, 0, ((data>>6) & 0x01));

	/* cassette data out */
	device_output(IO_CASSETTE, 0, (data & (1<<7)) ? -32768 : 32767);


	/* PRINTER STROBE */
	printer_handshake = 0;

	/* normal value is 1, 0 is the strobe */
	if ((data & (1<<4))!=0)
	{
		printer_handshake = CENTRONICS_STROBE;
	}

	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);
	centronics_write_handshake(0, printer_handshake, CENTRONICS_STROBE);
	
	oric_psg_connection_refresh();
	previous_portb_data = data;

}

static void oric_printer_handshake_in(int number, int data, int mask)
{
	int acknowledge;

	acknowledge = 1;

	if (mask & CENTRONICS_ACKNOWLEDGE)
	{
		if (data & CENTRONICS_ACKNOWLEDGE)
		{
			acknowledge = 0;
		}
	}

    via_set_input_ca1(0, acknowledge);
}

static CENTRONICS_CONFIG oric_cent_config[1]={
	{
		PRINTER_CENTRONICS,
		oric_printer_handshake_in
	},
};


READ_HANDLER ( oric_via_in_ca2_func )
{
	return oric_psg_control & 1;
}

READ_HANDLER ( oric_via_in_cb2_func )
{
	return (oric_psg_control>>1) & 1;
}

WRITE_HANDLER ( oric_via_out_ca2_func )
{
	oric_psg_control &=~1;

	if (data)
	{
		oric_psg_control |=1;
	}

	oric_psg_connection_refresh();
}

WRITE_HANDLER ( oric_via_out_cb2_func )
{
	oric_psg_control &=~2;

	if (data)
	{
		oric_psg_control |=2;
	}

	oric_psg_connection_refresh();
}


void	oric_via_irq_func(int state)
{
	oric_irqs &= ~(1<<0);

	if (state)
	{
		logerror("oric via1 interrupt\n");

		oric_irqs |=(1<<0);
	}

	oric_refresh_ints();
}


/*
VIA Lines
 Oric usage

PA0..PA7
 PSG data bus, printer data lines

CA1
 printer acknowledge line

CA2
 PSG BC1 line

PB0..PB2
 keyboard lines-demultiplexer

PB3
 keyboard sense line

PB4
 printer strobe line

PB5
 (not connected)

PB6
 tape connector motor control

PB7
 tape connector output

CB1
 tape connector input

CB2
 PSG BDIR line

*/

struct via6522_interface oric_6522_interface=
{
	oric_via_in_a_func,
	oric_via_in_b_func,
	NULL,				/* printer acknowledge - handled by callback*/
	NULL,				/* tape input - handled by timer */
	oric_via_in_ca2_func,
	oric_via_in_cb2_func,
	oric_via_out_a_func,
	oric_via_out_b_func,
	oric_via_out_ca2_func,
	oric_via_out_cb2_func,
	oric_via_irq_func,
	NULL,
	NULL,
	NULL,
	NULL
};

/* MICRODISC INTERFACE variables */
/* bit 7 is intrq state */
unsigned char port_314_r;
/* bit 7 is drq state (active low) */
unsigned char port_318_r;
/* bit 6,5: drive */
/* bit 4: side */
/* bit 3: double density enable */
/* bit 0: enable FDC IRQ to trigger IRQ on CPU */
unsigned char port_314_w;

static unsigned char oric_wd179x_int_state = 0;


static void oric_microdisc_refresh_wd179x_ints(void)
{
	oric_irqs &=~(1<<1);

	if ((oric_wd179x_int_state) && (port_314_w & (1<<0)))
	{
		logerror("oric microdisc interrupt\n");

		oric_irqs |=(1<<1);
	}

	oric_refresh_ints();
}

static void oric_microdisc_wd179x_callback(int State)
{
	switch (State)
	{
		case WD179X_IRQ_CLR:
		{
			port_314_r |=(1<<7);

			oric_wd179x_int_state = 0;

			oric_microdisc_refresh_wd179x_ints();
		}
		break;

		case WD179X_IRQ_SET:
		{
			port_314_r &= ~(1<<7);

			oric_wd179x_int_state = 1;

			oric_microdisc_refresh_wd179x_ints();
		}
		break;

		case WD179X_DRQ_CLR:
		{
			port_318_r |= (1<<7);
		}
		break;

		case WD179X_DRQ_SET:
		{
			port_318_r &=~(1<<7);
		}
		break;
	}
}

static void oric_wd179x_callback(int State)
{
	oric_microdisc_wd179x_callback(State);
}

int oric_floppy_init(int id)
{
	int result;

	/* attempt to open mfm disk */
	result = mfm_disk_floppy_init(id);

	if (result==INIT_OK)
	{
		oric_floppy_type[id] = ORIC_FLOPPY_MFM_DISK;

		return INIT_OK;
	}

	if (basicdsk_floppy_init(id))
	{
		/* I don't know what the geometry of the disc image should be, so the
		default is 80 tracks, 2 sides, 9 sectors per track */
		basicdsk_set_geometry(id, 80, 2, 9, 512, 1);

		oric_floppy_type[id] = ORIC_FLOPPY_BASIC_DISK;

		return INIT_OK;
	}

	return INIT_FAILED;
}

void	oric_floppy_exit(int id)
{
	switch (oric_floppy_type[id])
	{
		case ORIC_FLOPPY_MFM_DISK:
		{
			mfm_disk_floppy_exit(id);
		}
		break;

		case ORIC_FLOPPY_BASIC_DISK:
		{
			basicdsk_floppy_exit(id);
		}
		break;

		default:
			break;
	}

	oric_floppy_type[id] = ORIC_FLOPPY_NONE;
}

int		oric_floppy_id(int id)
{
	int result;

	/* check if it's a mfm disk first */
	result = mfm_disk_floppy_id(id);

	if (result==1)
		return 1;

	return basicdsk_floppy_id(id);
}

void	oric_microdisc_set_mem_0x0c000(void)
{
	if (oric_is_telestrat)
		return;

	/* for 0x0c000-0x0dfff: */
	/* if os disabled, ram takes priority */
	/* /ROMDIS */
	if ((port_314_w & (1<<1))==0)
	{
/*		logerror("&c000-&dfff is ram\n"); */
		/* rom disabled enable ram */
		memory_set_bankhandler_r(1, 0, MRA_BANK1);
		memory_set_bankhandler_w(5, 0, MWA_BANK5);
		cpu_setbank(1, oric_ram_0x0c000);
		cpu_setbank(5, oric_ram_0x0c000);
	}
	else
	{
		unsigned char *rom_ptr;
/*		logerror("&c000-&dfff is os rom\n"); */
		/* basic rom */
		memory_set_bankhandler_r(1, 0, MRA_BANK1);
		memory_set_bankhandler_w(5, 0, MWA_NOP);
		rom_ptr = memory_region(REGION_CPU1) + 0x010000;
		cpu_setbank(1, rom_ptr);
		cpu_setbank(5, rom_ptr);
	}

	/* for 0x0e000-0x0ffff */
	/* if not disabled, os takes priority */
	if ((port_314_w & (1<<1))!=0)
	{
		unsigned char *rom_ptr;
/*		logerror("&e000-&ffff is os rom\n"); */
		/* basic rom */
		memory_set_bankhandler_r(2, 0, MRA_BANK2);
		memory_set_bankhandler_w(6, 0, MWA_NOP);
		rom_ptr = memory_region(REGION_CPU1) + 0x010000;
		cpu_setbank(2, rom_ptr+0x02000);
		cpu_setbank(6, rom_ptr+0x02000);
	}
	else
	{
		/* if eprom is enabled, it takes priority over ram */
		if ((port_314_w & (1<<7))==0)
		{
			unsigned char *rom_ptr;
/*			logerror("&e000-&ffff is disk rom\n"); */
			memory_set_bankhandler_r(2, 0, MRA_BANK2);
			memory_set_bankhandler_w(6, 0, MWA_NOP);

			/* enable rom of microdisc interface */
			rom_ptr = memory_region(REGION_CPU1) + 0x014000;
			cpu_setbank(2, rom_ptr);
		}
		else
		{
/*			logerror("&e000-&ffff is ram\n"); */
			/* rom disabled enable ram */
			memory_set_bankhandler_r(2, 0, MRA_BANK2);
			memory_set_bankhandler_w(6, 0, MWA_BANK6);
			cpu_setbank(2, oric_ram_0x0c000+0x02000);
			cpu_setbank(6, oric_ram_0x0c000+0x02000);
		}
	}
}

void oric_common_init_machine(void)
{
    /* clear all irqs */
	oric_irqs = 0;

	oric_ram_0x0c000 = NULL;

    oric_tape_timer = timer_pulse(TIME_IN_HZ(11025), 0, oric_refresh_tape);

	/* disable os rom, enable microdisc rom */
	/* 0x0c000-0x0dfff will be ram, 0x0e000-0x0ffff will be microdisc rom */
    port_314_w = 0x0ff^((1<<7) | (1<<1));

	via_reset();
	via_config(0, &oric_6522_interface);
	via_set_clock(0,1000000);


	centronics_config(0, oric_cent_config);
	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);
    via_set_input_ca1(0, 1);

	wd179x_init(oric_wd179x_callback);
#ifdef ORIC_DUMP_RAM
	previous_input_port5 = readinputport(5);
#endif
}


void oric_init_machine (void)
{
	oric_common_init_machine();

	oric_is_telestrat = 0;

	oric_ram_0x0c000 = malloc(16384);

	if (readinputport(9) & 0x01)
	{
		/* disable os rom, enable microdisc rom */
		/* 0x0c000-0x0dfff will be ram, 0x0e000-0x0ffff will be microdisc rom */
		port_314_w = 0x0ff^((1<<7) | (1<<1));
	}
	else
	{
		/* disable microdisc rom, enable os rom */
		port_314_w = 0x0ff;
	}

	oric_microdisc_set_mem_0x0c000();
}

void oric_shutdown_machine (void)
{
	if (oric_ram_0x0c000)
		free(oric_ram_0x0c000);
	oric_ram_0x0c000 = NULL;
	wd179x_exit();

	if (oric_tape_timer)
	{
		timer_remove(oric_tape_timer);
		oric_tape_timer = NULL;
	}
}

READ_HANDLER (oric_microdisc_r)
{
	unsigned char data = 0x0ff;

	switch (offset & 0x0ff)
	{
		/* microdisc floppy disc interface */
		case 0x010:
			data = wd179x_status_r(0);
			break;
		case 0x011:
			data =wd179x_track_r(0);
			break;
		case 0x012:
			data = wd179x_sector_r(0);
			break;
		case 0x013:
			data = wd179x_data_r(0);
			break;
		case 0x014:
			data = port_314_r | 0x07f;
/*			logerror("port_314_r: %02x\n",data); */
			break;
		case 0x018:
			data = port_318_r | 0x07f;
/*			logerror("port_318_r: %02x\n",data); */
			break;

		default:
			data = via_0_r(offset & 0x0f);
			break;

	}

	return data;
}

WRITE_HANDLER(oric_microdisc_w)
{
	switch (offset & 0x0ff)
	{
		/* microdisc floppy disc interface */
		case 0x010:
			wd179x_command_w(0,data);
			break;
		case 0x011:
			wd179x_track_w(0,data);
			break;
		case 0x012:
			wd179x_sector_w(0,data);
			break;
		case 0x013:
			wd179x_data_w(0,data);
			break;
		case 0x014:
		{
			int density;

			port_314_w = data;

			logerror("port_314_w: %02x\n",data); 

			/* bit 6,5: drive */
			/* bit 4: side */
			/* bit 3: double density enable */
			/* bit 0: enable FDC IRQ to trigger IRQ on CPU */
			wd179x_set_drive((data>>5) & 0x03);
			wd179x_set_side((data>>4) & 0x01);
			if (data & (1<<3))
			{
				density = DEN_MFM_LO;
			}
			else
			{
				density = DEN_FM_HI;
			}

			wd179x_set_density(density);

			oric_microdisc_set_mem_0x0c000();
			oric_microdisc_refresh_wd179x_ints();
		}
		break;

		default:
			via_0_w(offset & 0x0f, data);
			break;
	}
}


READ_HANDLER ( oric_IO_r )
{
	if (readinputport(9) & 0x01)
	{
		if ((offset>=0x010) && (offset<=0x01f))
		{
			return oric_microdisc_r(offset);
		}
	}

	logerror("via 0 r: %04x\n",offset);

	/* it is repeated */
	return via_0_r(offset & 0x0f);
}

WRITE_HANDLER ( oric_IO_w )
{
	if (readinputport(9) & 0x01)
	{
		if ((offset>=0x010) && (offset<=0x01f))
		{
			oric_microdisc_w(offset, data);
			return;
		}
	}

	logerror("via 0 w: %04x %02x\n",offset,data);

	via_0_w(offset & 0x0f,data);
}


int oric_cassette_init(int id)
{
	void *file;

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
	if (file)
	{
		struct wave_args wa = {0,};
		wa.file = file;
		wa.display = 1;

		if (device_open(IO_CASSETTE, id, 0, &wa))
			return INIT_FAILED;

		return INIT_OK;
	}

	/* HJB 02/18: no file, create a new file instead */
	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_WRITE);
	if (file)
	{
		struct wave_args wa = {0,};
		wa.file = file;
		wa.display = 1;
		wa.smpfreq = 22050; /* maybe 11025 Hz would be sufficient? */
		/* open in write mode */
        if (device_open(IO_CASSETTE, id, 1, &wa))
            return INIT_FAILED;
		return INIT_OK;
    }

	return INIT_FAILED;
}

void oric_cassette_exit(int id)
{
	device_close(IO_CASSETTE, id);
}


/**** TELESTRAT ****/

/*
VIA lines
 Telestrat usage

PA0..PA2
 Memory bank selection

PA3
 "Midi" port pin 3

PA4
 RS232/Minitel selection

PA5
 Third mouse button (right joystick port pin 5)

PA6
 "Midi" port pin 5

PA7
 Second mouse button (right joystick port pin 9)

CA1
 "Midi" port pin 1

CA2
 not used ?

PB0..PB4
 Joystick ports

PB5
 Joystick doubler switch

PB6
 Select Left Joystick port

PB7
 Select Right Joystick port

CB1
 Phone Ring detection

CB2
 "Midi" port pin 4

*/

static unsigned char telestrat_bank_selection;
static unsigned char telestrat_via2_port_a_data;
static unsigned char telestrat_via2_port_b_data;

enum
{
	TELESTRAT_MEM_BLOCK_UNDEFINED,
	TELESTRAT_MEM_BLOCK_RAM,
	TELESTRAT_MEM_BLOCK_ROM
};

struct  telestrat_mem_block
{
	int		MemType;
	unsigned char *ptr;
};


static struct telestrat_mem_block	telestrat_blocks[8];

static void	telestrat_refresh_mem(void)
{
	struct telestrat_mem_block *mem_block = &telestrat_blocks[telestrat_bank_selection];

	switch (mem_block->MemType)
	{
		case TELESTRAT_MEM_BLOCK_RAM:
		{
			memory_set_bankhandler_r(1, 0, MRA_BANK1);
			memory_set_bankhandler_w(2, 0, MWA_BANK2);
			cpu_setbank(1, mem_block->ptr);
			cpu_setbank(2, mem_block->ptr);
		}
		break;

		case TELESTRAT_MEM_BLOCK_ROM:
		{
			memory_set_bankhandler_r(1, 0, MRA_BANK1);
			memory_set_bankhandler_w(2, 0, MWA_NOP);
			cpu_setbank(1, mem_block->ptr);
		}
		break;

		default:
		case TELESTRAT_MEM_BLOCK_UNDEFINED:
		{
			memory_set_bankhandler_r(1, 0, MRA_NOP);
			memory_set_bankhandler_w(2, 0, MWA_NOP);
		}
		break;
	}
}

static READ_HANDLER(telestrat_via2_in_a_func)
{
	logerror("via 2 - port a %02x\n",telestrat_via2_port_a_data);
	return telestrat_via2_port_a_data;
}


static WRITE_HANDLER(telestrat_via2_out_a_func)
{
	logerror("via 2 - port a w: %02x\n",data);

	telestrat_via2_port_a_data = data;

	if (((data^telestrat_bank_selection) & 0x07)!=0)
	{
		telestrat_bank_selection = data & 0x07;

		telestrat_refresh_mem();
	}
}

static READ_HANDLER(telestrat_via2_in_b_func)
{
	unsigned char data = 0x01f;

	/* left joystick selected? */
	if (telestrat_via2_port_b_data & (1<<6))
	{
		data &= readinputport(9);
	}

	/* right joystick selected? */
	if (telestrat_via2_port_b_data & (1<<7))
	{
		data &= readinputport(10);
	}

	data |= telestrat_via2_port_b_data & ((1<<7) | (1<<6) | (1<<5));

	return data;
}

static WRITE_HANDLER(telestrat_via2_out_b_func)
{
	telestrat_via2_port_b_data = data;
}


void	telestrat_via2_irq_func(int state)
{
    oric_irqs &=~(1<<2);

	if (state)
	{
        logerror("telestrat via2 interrupt\n");

        oric_irqs |=(1<<2);
	}

    oric_refresh_ints();
}
struct via6522_interface telestrat_via2_interface=
{
	telestrat_via2_in_a_func,
	telestrat_via2_in_b_func,
	NULL,
	NULL,
	NULL,
	NULL,
	telestrat_via2_out_a_func,
	telestrat_via2_out_b_func,
	NULL,
	NULL,
	telestrat_via2_irq_func,
	NULL,
	NULL,
	NULL,
	NULL
};


void telestrat_init_machine(void)
{
	oric_common_init_machine();

	oric_is_telestrat = 1;

	/* initialise overlay ram */
	telestrat_blocks[0].MemType = TELESTRAT_MEM_BLOCK_RAM;
	telestrat_blocks[0].ptr = (unsigned char *)malloc(16384);

	telestrat_blocks[1].MemType = TELESTRAT_MEM_BLOCK_RAM;
	telestrat_blocks[1].ptr = (unsigned char *)malloc(16384);
	telestrat_blocks[2].MemType = TELESTRAT_MEM_BLOCK_RAM;
	telestrat_blocks[2].ptr = (unsigned char *)malloc(16384);

	/* initialise default cartridge */
	telestrat_blocks[3].MemType = TELESTRAT_MEM_BLOCK_ROM;
	telestrat_blocks[3].ptr = memory_region(REGION_CPU1)+0x010000;

	telestrat_blocks[4].MemType = TELESTRAT_MEM_BLOCK_RAM;
	telestrat_blocks[4].ptr = (unsigned char *)malloc(16384);

	/* initialise default cartridge */
	telestrat_blocks[5].MemType = TELESTRAT_MEM_BLOCK_ROM;
	telestrat_blocks[5].ptr = memory_region(REGION_CPU1)+0x014000;

	/* initialise default cartridge */
	telestrat_blocks[6].MemType = TELESTRAT_MEM_BLOCK_ROM;
	telestrat_blocks[6].ptr = memory_region(REGION_CPU1)+0x018000;

	/* initialise default cartridge */
	telestrat_blocks[7].MemType = TELESTRAT_MEM_BLOCK_ROM;
	telestrat_blocks[7].ptr = memory_region(REGION_CPU1)+0x01c000;

	telestrat_bank_selection = 7;
	telestrat_refresh_mem();


	via_config(1, &telestrat_via2_interface);
	via_set_clock(1,1000000);
}

void	telestrat_shutdown_machine(void)
{
	int i;

	/* this supports different cartridges now. In the init machine
	a cartridge is hard-coded, but if other cartridges exist it could
	be changed above */
	for (i=0; i<8; i++)
	{
		if (telestrat_blocks[i].MemType == TELESTRAT_MEM_BLOCK_RAM)
		{
			if (telestrat_blocks[i].ptr!=NULL)
			{
				free(telestrat_blocks[i].ptr);
				telestrat_blocks[i].ptr = NULL;
			}
		}
	}

	oric_shutdown_machine();
}


WRITE_HANDLER(telestrat_IO_w)
{
	if (offset<0x010)
	{
		via_0_w(offset & 0x0f,data);
		return;
	}

	if ((offset>=0x020) && (offset<=0x02f))
	{
		via_1_w(offset & 0x0f,data);
		return;
	}

	if ((offset>=0x010) && (offset<=0x01b))
	{
		oric_microdisc_w(offset, data);
		return;
	}

	if ((offset>=0x01c) && (offset<=0x01f))
	{
		acia_6551_w(offset, data);
		return;
	}
	
	logerror("null write %04x %02x\n",offset,data);

}

READ_HANDLER(telestrat_IO_r)
{
	if (offset<0x010)
	{
		return via_0_r(offset & 0x0f);
	}

	if ((offset>=0x020) && (offset<=0x02f))
	{
		return via_1_r(offset & 0x0f);
	}

	if ((offset>=0x010) && (offset<=0x01b))
	{
		return oric_microdisc_r(offset);
	}

	if ((offset>=0x01c) && (offset<=0x01f))
	{
		return acia_6551_r(offset);
	}

	logerror("null read %04x\n",offset);
	return 0x0ff;
}



