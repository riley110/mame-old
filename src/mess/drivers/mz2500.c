/***************************************************************************

    Sharp MZ-2500 (c) 1985 Sharp Corporation

    preliminary driver by Angelo Salese

	TODO:
	- floppy device;
	- graphics are really a bare minimum, this system is very complex;
	- keyboard inputs (needs something that actually works);

    memory map:
    0x00000-0x3ffff Work RAM
    0x40000-0x5ffff CG RAM (address bitswapped?)
    0x60000-0x67fff "Read modify write" area (related to the CG RAM) (0x30-0x33)
    0x68000-0x6ffff IPL ROM (0x34-0x37)
    0x70000-0x71fff TVRAM (0x38)
    0x72000-0x73fff Kanji ROM / PCG RAM (banked) (0x39)
    0x74000-0x75fff Dictionary ROM (banked) (0x3a)
    0x76000-0x77fff NOP (0x3b)
    0x78000-0x7ffff Phone ROM (0x3c-0x3f)

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/z80pio.h"
#include "machine/i8255a.h"
#include "machine/wd17xx.h"
#include "machine/pit8253.h"
#include "sound/2203intf.h"

//#include "devices/cassette.h"
#include "devices/flopdrv.h"
#include "formats/flopimg.h"
#include "formats/basicdsk.h"

/* machine stuff */
static UINT8 bank_val[8],bank_addr;
static UINT8 irq_sel,irq_vector[4],irq_mask[4];
static UINT8 kanji_bank;
static UINT8 fdc_reverse;
static UINT8 key_mux;
static UINT8 cg_reg_index;
static UINT8 cg_reg[0x20];
static UINT8 clut16[0x10];
static UINT16 clut256[0x100];

#define WRAM_RESET 0
#define IPL_RESET 1

static void mz2500_reset(UINT8 type);
static const UINT8 bank_reset_val[2][8] =
{
	{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 },
	{ 0x34, 0x35, 0x36, 0x37, 0x04, 0x05, 0x06, 0x07 }
};

/* video stuff*/
static UINT8 text_reg[0x100], text_reg_index;
static UINT8 text_col_size, text_font_reg;
static UINT8 pal_select;

static VIDEO_START( mz2500 )
{
}

static void draw_80x25(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect)
{
	UINT8 *vram = memory_region(machine, "maincpu");
	int x,y,count,xi,yi;
	UINT8 *gfx_data;
	UINT8 y_step;

	count = 0x70000;

	y_step = (text_font_reg) ? 1 : 2;

	for (y=0;y<25;y+=y_step)
	{
		for (x=0;x<80;x++)
		{
			int tile = vram[count+0x0000] & 0xff;
			int attr = vram[count+0x0800];
			int tile_bank = vram[count+0x1000] & 0x3f;
			int gfx_sel = (attr & 0x38) | (vram[count+0x1000] & 0xc0);
			//int gfx_num;
			int color = attr & 7;

			if(gfx_sel == 0x80 || gfx_sel == 0xc0)
				gfx_data = memory_region(machine,"kanji"); //TODO
			else
				gfx_data = memory_region(machine,"pcg");

			tile|= tile_bank << 8;

			for(yi=0;yi<8*y_step;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					UINT8 pen_bit[3],pen;

					if(gfx_sel & 0x8)
					{
						pen_bit[0] = ((gfx_data[tile*8+yi+0x1800]>>(7-xi)) & 1) ? 4 : 0; //G
						pen_bit[1] = ((gfx_data[tile*8+yi+0x1000]>>(7-xi)) & 1) ? 2 : 0; //R
						pen_bit[2] = ((gfx_data[tile*8+yi+0x0800]>>(7-xi)) & 1) ? 1 : 0; //B

						pen = (pen_bit[0]|pen_bit[1]|pen_bit[2]);
					}
					else
						pen = ((gfx_data[tile*8+yi]>>(7-xi)) & 1) ? color : 0;

					*BITMAP_ADDR16(bitmap, (y*8+yi), x*8+xi) = machine->pens[pen];
				}
			}
				//drawgfx_opaque(bitmap,cliprect,screen->machine->gfx[gfx_num],tile,color,0,0,x*8,(y)*8);

			count++;
		}
	}
}

static void draw_40x25(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect,int plane)
{
	UINT8 *vram = memory_region(machine, "maincpu");
	int x,y,count,xi,yi;
	UINT8 *gfx_data;
	UINT8 y_step;

	count = 0x70000 + (plane * 0x400);

	y_step = (text_font_reg) ? 1 : 2;

	for (y=0;y<25;y+=y_step)
	{
		for (x=0;x<40;x++)
		{
			int tile = vram[count+0x0000] & 0xff;
			int attr = vram[count+0x0800];
			int tile_bank = vram[count+0x1000] & 0x3f;
			int gfx_sel = (attr & 0x38) | (vram[count+0x1000] & 0xc0);
			//int gfx_num;
			int color = attr & 7;

			if(gfx_sel == 0x80 || gfx_sel == 0xc0)
				gfx_data = memory_region(machine,"kanji"); //TODO
			else
				gfx_data = memory_region(machine,"pcg");

			tile|= tile_bank << 8;

			for(yi=0;yi<8*y_step;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					UINT8 pen_bit[3],pen;

					if(gfx_sel & 0x8)
					{
						pen_bit[0] = ((gfx_data[tile*8+yi+0x1800]>>(7-xi)) & 1) ? 4 : 0; //G
						pen_bit[1] = ((gfx_data[tile*8+yi+0x1000]>>(7-xi)) & 1) ? 2 : 0; //R
						pen_bit[2] = ((gfx_data[tile*8+yi+0x0800]>>(7-xi)) & 1) ? 1 : 0; //B

						pen = (pen_bit[0]|pen_bit[1]|pen_bit[2]);
					}
					else
					{
						if((gfx_sel & 0x30) == 0x30)
							pen = ((gfx_data[tile*8+yi+0x1800]>>(7-xi)) & 1) ? color : 0; //G
						else if((gfx_sel & 0x30) == 0x20)
							pen = ((gfx_data[tile*8+yi+0x1000]>>(7-xi)) & 1) ? color : 0; //R
						else if((gfx_sel & 0x30) == 0x10)
							pen = ((gfx_data[tile*8+yi+0x0800]>>(7-xi)) & 1) ? color : 0; //B
						else
							pen = ((gfx_data[tile*8+yi+0x0000]>>(7-xi)) & 1) ? color : 0;
					}


					if(pen)
						*BITMAP_ADDR16(bitmap, (y*8+yi), x*8+xi) = machine->pens[pen];
				}
			}
				//drawgfx_opaque(bitmap,cliprect,screen->machine->gfx[gfx_num],tile,color,0,0,x*8,(y)*8);

			count++;
		}
	}
}

static void draw_tv_screen(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect)
{
	//w = (text_col_size) ? 2 : 1; //80 : 40
	if(text_col_size)
		draw_80x25(machine,bitmap,cliprect);
	else
	{
		int tv_mode;

		tv_mode = text_reg[0] >> 2;

		switch(tv_mode & 3)
		{
//			case 0: mixed 6bpp mode
			case 1:
				draw_40x25(machine,bitmap,cliprect,0);
				break;
			case 2:
				draw_40x25(machine,bitmap,cliprect,1);
				break;
			case 3:
				draw_40x25(machine,bitmap,cliprect,0);
				draw_40x25(machine,bitmap,cliprect,1);
				break;
			default: popmessage("%02x %02x %02x",tv_mode & 3,text_reg[1],text_reg[2]); break;
		}
	}

	#if 0
	if(text_font_reg)
	{

	}
	else
	{
		for (y=0;y<25;y+=2)
		{
			for (x=0;x<40*w;x++)
			{
				int tile = vram[count+0x0000] & 0xfe;
				int attr = vram[count+0x0800];
				int tile_bank = vram[count+0x1000] & 0x3f;
				int gfx_sel = (attr & 0x38) | (vram[count+0x1000] & 0xc0);
				//int gfx_num;
				int color = attr & 7;

				if(gfx_sel == 0x80 || gfx_sel == 0xc0)
					gfx_data = memory_region(machine,"kanji"); //TODO
				else
					gfx_data = memory_region(machine,"pcg");

				tile|= tile_bank << 8;

				for(yi=0;yi<16;yi++)
				{
					for(xi=0;xi<8;xi++)
					{
						UINT8 pen;

						pen = ((gfx_data[tile*8+yi]>>(7-xi)) & 1) ? color : 0;

						*BITMAP_ADDR16(bitmap, (y*8+yi), x*8+xi) = machine->pens[pen];
					}
				}

//				drawgfx_opaque(bitmap,cliprect,screen->machine->gfx[gfx_num],tile,color,0,0,x*8,(y)*8);
//				drawgfx_opaque(bitmap,cliprect,screen->machine->gfx[gfx_num],tile+1,color,0,0,x*8,(y+1)*8);

				count++;
			}
		}
	}
	#endif
}

static void draw_cg16_screen(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect,int plane,int x_size)
{
	static UINT32 count;
	UINT8 *vram = memory_region(machine, "maincpu");
	UINT8 pen,pen_bit[4];
	int x,y,xi,pen_i;

	count = 0x40000;

	for(y=0;y<200;y++)
	{
		for(x=0;x<x_size;x+=8)
		{
			for(xi=0;xi<8;xi++)
			{
				pen_bit[0] = (vram[count+0x0000 + plane*0x2000]>>(xi)) & 1 ? 1 : 0; //B
				pen_bit[1] = (vram[count+0x4000 + plane*0x2000]>>(xi)) & 1 ? 2 : 0; //R
				pen_bit[2] = (vram[count+0x8000 + plane*0x2000]>>(xi)) & 1 ? 4 : 0; //G
				pen_bit[3] = (vram[count+0xc000 + plane*0x2000]>>(xi)) & 1 ? 8 : 0; //I

				pen = 0;
				for(pen_i=0;pen_i<4;pen_i++)
					pen |= pen_bit[pen_i];

				if(pen != 0 || clut16[pen] & 0x10)
					*BITMAP_ADDR16(bitmap, y, x+xi) = machine->pens[(clut16[pen] & 0x0f)+0x200];
			}
			count++;
		}
	}
}

/* TODO: this probably allocates CLUT to somewhere ... */
static void draw_cg256_screen(running_machine *machine, bitmap_t *bitmap,const rectangle *cliprect)
{
	static UINT32 count;
	UINT8 *vram = memory_region(machine, "maincpu");
	UINT8 pen,pen_bit[8];
	int x,y,xi,pen_i;

	count = 0x40000;

	for(y=0;y<200;y++)
	{
		for(x=0;x<320;x+=8)
		{
			for(xi=0;xi<8;xi++)
			{
				pen_bit[0] = (vram[count + 0x2000]>>(xi)) & 1 ? (cg_reg[0x18] & 0x10) : 0; // B1
				pen_bit[1] = (vram[count + 0x0000]>>(xi)) & 1 ? (cg_reg[0x18] & 0x01) : 0; // B0
				pen_bit[2] = (vram[count + 0x6000]>>(xi)) & 1 ? (cg_reg[0x18] & 0x20) : 0; // R1
				pen_bit[3] = (vram[count + 0x4000]>>(xi)) & 1 ? (cg_reg[0x18] & 0x02) : 0; // R0
				pen_bit[4] = (vram[count + 0xa000]>>(xi)) & 1 ? (cg_reg[0x18] & 0x40) : 0; // G1
				pen_bit[5] = (vram[count + 0x8000]>>(xi)) & 1 ? (cg_reg[0x18] & 0x04) : 0; // G0
				pen_bit[6] = (vram[count + 0xe000]>>(xi)) & 1 ? (cg_reg[0x18] & 0x80) : 0; // I1
				pen_bit[7] = (vram[count + 0xc000]>>(xi)) & 1 ? (cg_reg[0x18] & 0x08) : 0; // I0

				pen = 0;
				for(pen_i=0;pen_i<8;pen_i++)
					pen |= pen_bit[pen_i];

				if(pen != 0 || clut256[pen] & 0x100)
					*BITMAP_ADDR16(bitmap, y, x+xi) = machine->pens[(clut256[pen] & 0xff)+0x100];
			}
			count++;
		}
	}
}

static VIDEO_UPDATE( mz2500 )
{
	bitmap_fill(bitmap, cliprect, screen->machine->pens[(clut16[0] & 0xf)+0x200]); //TODO: correct?

	if(pal_select)
		draw_tv_screen(screen->machine,bitmap,cliprect);
	else //4096 mode colors
	{
		// ...
	}


	switch(cg_reg[0x0e])
	{
		case 0x03: /* CG 4 color mode */ break;
		case 0x14:
			draw_cg16_screen(screen->machine,bitmap,cliprect,0,320);
			draw_cg16_screen(screen->machine,bitmap,cliprect,1,320);
			break;
		case 0x15:
			draw_cg16_screen(screen->machine,bitmap,cliprect,1,320);
			draw_cg16_screen(screen->machine,bitmap,cliprect,0,320);
			break;
		case 0x17:
			draw_cg16_screen(screen->machine,bitmap,cliprect,0,640);
			break;
		case 0x1d:
			draw_cg256_screen(screen->machine,bitmap,cliprect);
			break;
	}

    return 0;
}

static UINT8 mz2500_ram_read(running_machine *machine, UINT16 offset, UINT8 bank_num)
{
	UINT8 *ram = memory_region(machine, "maincpu");
	UINT8 cur_bank = bank_val[bank_num];

	switch(cur_bank)
	{
		case 0x39:
		{
			if(kanji_bank & 0x80) //kanji ROM
			{
				UINT8 *knj_rom = memory_region(machine, "kanji");

				return knj_rom[(offset & 0x7ff)+((kanji_bank & 0x7f)*0x800)];
			}
			else //PCG RAM
			{
				UINT8 *pcg_ram = memory_region(machine, "pcg");

				return pcg_ram[offset];
			}
		}
		break;
		default: return ram[offset+cur_bank*0x2000];
	}

	return 0xff;
}

static void mz2500_ram_write(running_machine *machine, UINT16 offset, UINT8 data, UINT8 bank_num)
{
	UINT8 *ram = memory_region(machine, "maincpu");
	UINT8 cur_bank = bank_val[bank_num];

//	if(cur_bank >= 0x30 && cur_bank <= 0x33)
//		printf("CG REG = %02x %02x %02x %02x | offset = %04x | data = %02x\n",cg_reg[0],cg_reg[1],cg_reg[2],cg_reg[3],offset,data);

	switch(cur_bank)
	{
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		{
			// READ MODIFY WRITE
			if(cg_reg[0x0e] == 0x3)
			{
				// ...
			}
			else
			{
				if((cg_reg[0x05] & 0xc0) == 0x00) //replace
				{
					if(cg_reg[5] & 1) //B
					{
						ram[offset+((cur_bank & 3)*0x2000)+0x40000] &= ~cg_reg[6];
						ram[offset+((cur_bank & 3)*0x2000)+0x40000] |= (cg_reg[4] & 1) ? (data & cg_reg[0] & cg_reg[6]) : 0;
					}
					if(cg_reg[5] & 2) //R
					{
						ram[offset+((cur_bank & 3)*0x2000)+0x44000] &= ~cg_reg[6];
						ram[offset+((cur_bank & 3)*0x2000)+0x44000] |= (cg_reg[4] & 2) ? (data & cg_reg[1] & cg_reg[6]) : 0;
					}
					if(cg_reg[5] & 4) //G
					{
						ram[offset+((cur_bank & 3)*0x2000)+0x48000] &= ~cg_reg[6];
						ram[offset+((cur_bank & 3)*0x2000)+0x48000] |= (cg_reg[4] & 4) ? (data & cg_reg[2] & cg_reg[6]) : 0;
					}
					if(cg_reg[5] & 8) //I
					{
						ram[offset+((cur_bank & 3)*0x2000)+0x4c000] &= ~cg_reg[6];
						ram[offset+((cur_bank & 3)*0x2000)+0x4c000] |= (cg_reg[4] & 8) ? (data & cg_reg[3] & cg_reg[6]) : 0;
					}
				}
				else if((cg_reg[0x05] & 0xc0) == 0x40) //pset
				{
					if(cg_reg[5] & 1) //B
					{
						ram[offset+((cur_bank & 3)*0x2000)+0x40000] &= ~data;
						ram[offset+((cur_bank & 3)*0x2000)+0x40000] |= (cg_reg[4] & 1) ? (data & cg_reg[0]) : 0;
					}
					if(cg_reg[5] & 2) //R
					{
						ram[offset+((cur_bank & 3)*0x2000)+0x44000] &= ~data;
						ram[offset+((cur_bank & 3)*0x2000)+0x44000] |= (cg_reg[4] & 2) ? (data & cg_reg[1]) : 0;
					}
					if(cg_reg[5] & 4) //G
					{
						ram[offset+((cur_bank & 3)*0x2000)+0x48000] &= ~data;
						ram[offset+((cur_bank & 3)*0x2000)+0x48000] |= (cg_reg[4] & 4) ? (data & cg_reg[2]) : 0;
					}
					if(cg_reg[5] & 8) //I
					{
						ram[offset+((cur_bank & 3)*0x2000)+0x4c000] &= ~data;
						ram[offset+((cur_bank & 3)*0x2000)+0x4c000] |= (cg_reg[4] & 8) ? (data & cg_reg[3]) : 0;
					}
				}
			}
			break;
		}
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		{
			// IPL ROM, WRITENOP
			//printf("%04x %02x\n",offset+bank_num*0x2000,data);
			break;
		}
		case 0x39:
		{
			if(kanji_bank & 0x80) //kanji ROM
			{
				//NOP
			}
			else //PCG RAM
			{
				UINT8 *pcg_ram = memory_region(machine, "pcg");
				pcg_ram[offset] = data;
				if((offset & 0x1800) == 0x0000)
					gfx_element_mark_dirty(machine->gfx[3], (offset) >> 3);
				else
					gfx_element_mark_dirty(machine->gfx[4], (offset & 0x7ff) >> 3);
			}
		}
		break;
		default: ram[offset+cur_bank*0x2000] = data; break;
	}
}

static READ8_HANDLER( bank0_r ) { return mz2500_ram_read(space->machine, offset, 0); }
static READ8_HANDLER( bank1_r ) { return mz2500_ram_read(space->machine, offset, 1); }
static READ8_HANDLER( bank2_r ) { return mz2500_ram_read(space->machine, offset, 2); }
static READ8_HANDLER( bank3_r ) { return mz2500_ram_read(space->machine, offset, 3); }
static READ8_HANDLER( bank4_r ) { return mz2500_ram_read(space->machine, offset, 4); }
static READ8_HANDLER( bank5_r ) { return mz2500_ram_read(space->machine, offset, 5); }
static READ8_HANDLER( bank6_r ) { return mz2500_ram_read(space->machine, offset, 6); }
static READ8_HANDLER( bank7_r ) { return mz2500_ram_read(space->machine, offset, 7); }
static WRITE8_HANDLER( bank0_w ) { mz2500_ram_write(space->machine, offset, data, 0); }
static WRITE8_HANDLER( bank1_w ) { mz2500_ram_write(space->machine, offset, data, 1); }
static WRITE8_HANDLER( bank2_w ) { mz2500_ram_write(space->machine, offset, data, 2); }
static WRITE8_HANDLER( bank3_w ) { mz2500_ram_write(space->machine, offset, data, 3); }
static WRITE8_HANDLER( bank4_w ) { mz2500_ram_write(space->machine, offset, data, 4); }
static WRITE8_HANDLER( bank5_w ) { mz2500_ram_write(space->machine, offset, data, 5); }
static WRITE8_HANDLER( bank6_w ) { mz2500_ram_write(space->machine, offset, data, 6); }
static WRITE8_HANDLER( bank7_w ) { mz2500_ram_write(space->machine, offset, data, 7); }


static READ8_HANDLER( mz2500_bank_addr_r )
{
	return bank_addr;
}

static WRITE8_HANDLER( mz2500_bank_addr_w )
{
//	printf("%02x\n",data);
	bank_addr = data & 7;
}

static READ8_HANDLER( mz2500_bank_data_r )
{
	static UINT8 res;

	res = bank_val[bank_addr];

	bank_addr++;
	bank_addr&=7;

	return res;
}

static WRITE8_HANDLER( mz2500_bank_data_w )
{
//  UINT8 *ROM = memory_region(space->machine, "maincpu");
//  static const char *const bank_name[] = { "bank0", "bank1", "bank2", "bank3", "bank4", "bank5", "bank6", "bank7" };

	bank_val[bank_addr] = data & 0x3f;

	//printf("%02x %02x\n",data & 0x3f,bank_addr);

//  if((data*2) >= 0x70)
//  printf("%s %02x\n",bank_name[bank_addr],bank_val[bank_addr]*2);

//  memory_set_bankptr(space->machine, bank_name[bank_addr], &ROM[bank_val[bank_addr]*0x2000]);

	bank_addr++;
	bank_addr&=7;
}

static WRITE8_HANDLER( mz2500_kanji_bank_w )
{
	kanji_bank = data;
}

/* 0xf4 - 0xf7 all returns vblank / hblank states */
static READ8_HANDLER( mz2500_crtc_hvblank_r )
{
	static UINT8 vblank_bit, hblank_bit;

	vblank_bit = space->machine->primary_screen->vblank() ? 0 : 1;
	hblank_bit = space->machine->primary_screen->hblank() ? 0 : 2;

	return vblank_bit | hblank_bit;
}

/*
[0] ---x ---- line height (0) 16 (1) 20
[0] ---- xx-- 40 column mode (0) 64 colors (1) screen 1 (2) screen 2 (3) screen 1 + screen 2
[1] xxxx xxxx TV map offset low address value
[2] ---- -xxx TV map offset high address value

[9] ---- xxxx vertical scrolling? (val * 2)
*/

static UINT8 pal_256_param(int index, int param)
{
	UINT8 val = 0;

	switch(param & 3)
	{
		case 0: val = index & 0x80 ? 1 : 0; break;
		case 1: val = index & 0x08 ? 1 : 0; break;
		case 2: val = 1; break;
		case 3: val = 0; break;
	}

	return val;
}

static WRITE8_HANDLER( mz2500_tv_crtc_w )
{
	switch(offset)
	{
		case 0: text_reg_index = data; break;
		case 1:
			text_reg[text_reg_index] = data;
			//printf("[%02x] %02x\n",text_reg_index,data);

			if(text_reg_index == 0x0a) // set 256 color palette
			{
				int i,r,g,b;
				UINT8 b_param,r_param,g_param;

				b_param = (data & 0x03) >> 0;
				r_param = (data & 0x0c) >> 2;
				g_param = (data & 0x30) >> 4;

				for(i = 0;i < 0x100;i++)
				{
					int bit0,bit1,bit2;

					bit0 = pal_256_param(i,b_param) ? 1 : 0;
					bit1 = i & 0x01 ? 2 : 0;
					bit2 = i & 0x10 ? 4 : 0;
					b = bit0|bit1|bit2;
					bit0 = pal_256_param(i,r_param) ? 1 : 0;
					bit1 = i & 0x02 ? 2 : 0;
					bit2 = i & 0x20 ? 4 : 0;
					r = bit0|bit1|bit2;
					bit0 = pal_256_param(i,g_param) ? 1 : 0;
					bit1 = i & 0x04 ? 2 : 0;
					bit2 = i & 0x40 ? 4 : 0;
					g = bit0|bit1|bit2;

					palette_set_color_rgb(space->machine, i+0x100,pal3bit(r),pal3bit(g),pal3bit(b));
				}

			}
			if(text_reg_index >= 0x80 && text_reg_index <= 0x8f) //Bitmap 16 clut registers
			{
				/*
				---x ---- priority
				---- xxxx clut number
				*/
				clut16[text_reg_index & 0xf] = data & 0x1f;

				{
					int i;

					for(i=0;i<0x10;i++)
					{
						clut256[(text_reg_index & 0xf) | (i << 4)] = (((data & 0x1f) << 4) | i);
					}
				}
			}
			//printf("[%02x]<- %02x\n",text_reg[text_reg_index],text_reg_index);
			break;
		case 2: /* CG MASK reg */ break;
		case 3:
			/* Font size reg */
			text_font_reg = data & 1;
			break;
	}
}

static WRITE8_HANDLER( mz2500_irq_sel_w )
{
	irq_sel = data;
	//printf("%02x\n",irq_sel);
	// activeness is trusted, see Tower of Druaga
	irq_mask[0] = (data & 0x08); //CRTC
	irq_mask[1] = (data & 0x04); //i8253
	irq_mask[2] = (data & 0x02); //printer
	irq_mask[3] = (data & 0x01); //RP5c15

	printf("%02x\n",data);
}

static WRITE8_HANDLER( mz2500_irq_data_w )
{
	if(irq_sel & 0x80)
		irq_vector[0] = data; //CRTC
	if(irq_sel & 0x40)
		irq_vector[1] = data; //i8253
	if(irq_sel & 0x20)
		irq_vector[2] = data; //printer
	if(irq_sel & 0x10)
		irq_vector[3] = data; //RP5c15

//	popmessage("%02x %02x %02x %02x",irq_vector[0],irq_vector[1],irq_vector[2],irq_vector[3]);
}

static WRITE8_HANDLER( mz2500_fdc_w )
{
	running_device* dev = space->machine->device("mb8877a");

	switch(offset+0xdc)
	{
		case 0xdc:
			wd17xx_set_drive(dev,data & 3);
			floppy_mon_w(floppy_get_device(space->machine, data & 3), (data & 0x80) ? ASSERT_LINE : CLEAR_LINE);
			floppy_drive_set_ready_state(floppy_get_device(space->machine, data & 3), 1,0);
			break;
		case 0xdd:
			wd17xx_set_side(dev,(data & 1));
			break;
	}
}

static const wd17xx_interface mz2500_mb8877a_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{FLOPPY_0, FLOPPY_1, FLOPPY_2, FLOPPY_3}
};

static FLOPPY_OPTIONS_START( mz2500 )
	FLOPPY_OPTION( img2d, "2d", "2D disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config mz2500_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_3_5_DSHD,
	FLOPPY_OPTIONS_NAME(default),
	NULL
};

static ADDRESS_MAP_START(mz2500_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x1fff) AM_READWRITE(bank0_r,bank0_w)
	AM_RANGE(0x2000, 0x3fff) AM_READWRITE(bank1_r,bank1_w)
	AM_RANGE(0x4000, 0x5fff) AM_READWRITE(bank2_r,bank2_w)
	AM_RANGE(0x6000, 0x7fff) AM_READWRITE(bank3_r,bank3_w)
	AM_RANGE(0x8000, 0x9fff) AM_READWRITE(bank4_r,bank4_w)
	AM_RANGE(0xa000, 0xbfff) AM_READWRITE(bank5_r,bank5_w)
	AM_RANGE(0xc000, 0xdfff) AM_READWRITE(bank6_r,bank6_w)
	AM_RANGE(0xe000, 0xffff) AM_READWRITE(bank7_r,bank7_w)
ADDRESS_MAP_END

#if 0
static READ8_HANDLER( kludge_r )
{
	return 0xff; //mame_rand(space->machine);
}
#endif

static UINT32 rom_index;
static UINT8 hrom_index,lrom_index;

static READ8_HANDLER( mz2500_rom_r )
{
	UINT8 *rom = memory_region(space->machine, "rom");
	UINT8 res;

	lrom_index = (cpu_get_reg(space->machine->device("maincpu"), Z80_B));

	rom_index = (rom_index & 0xffff00) | (lrom_index & 0xff);

	res = rom[rom_index];

	return res;
}

static WRITE8_HANDLER( mz2500_rom_w )
{
	hrom_index = (cpu_get_reg(space->machine->device("maincpu"), Z80_B));

	rom_index = (data << 8) | (rom_index & 0x0000ff) | ((hrom_index & 0xff)<<16);
	//printf("%02x\n",data);
}

/* sets first 16 color entries of the 4096 palette bank */
static WRITE8_HANDLER( palette4096_io_w )
{
	static UINT8 r[16],g[16],b[16];
	static UINT8 pal_index;
	static UINT8 pal_entry;

	pal_index = cpu_get_reg(space->machine->device("maincpu"), Z80_B);
	pal_entry = (pal_index & 0x1e) >> 1;

	if(pal_index & 1)
		g[pal_entry] = (data & 0x0f);
	else
	{
		r[pal_entry] = (data & 0xf0) >> 4;
		b[pal_entry] = data & 0x0f;
	}

	palette_set_color_rgb(space->machine, pal_entry+0x200,pal4bit(r[pal_entry]),pal4bit(g[pal_entry]),pal4bit(b[pal_entry]));
}

static READ8_DEVICE_HANDLER( mz2500_wd17xx_r )
{
	return wd17xx_r(device, offset) ^ fdc_reverse;
}

static WRITE8_DEVICE_HANDLER( mz2500_wd17xx_w )
{
	wd17xx_w(device, offset, data ^ fdc_reverse);
}

/*
"GDE" CRTC registers

0x05: clear bitmap buffer register

0x08: screen res vertical start lo reg
0x09: screen res vertical start hi reg (upper 1 bit)
0x0a: screen res vertical end lo reg
0x0b: screen res vertical end hi reg (upper 1 bit)
0x0c: screen res horizontal start reg (7 bits, val x 8)
0x0d: screen res horizontal end reg (7 bits, val x 8)
0x0e: screen size reg

*/

static READ8_HANDLER( mz2500_rplane_latch_r )
{
	if(cg_reg[0x07] & 0x10)
	{
		static UINT8 vblank_bit;

		// ---- ---x clear flag
		vblank_bit = space->machine->primary_screen->vblank() ? 0 : 0x80;

		return vblank_bit;
	}
	else
		return 0xff;
}

static WRITE8_HANDLER( mz2500_cg_addr_w )
{
	cg_reg_index = data;
}

static WRITE8_HANDLER( mz2500_cg_data_w )
{
	cg_reg[cg_reg_index & 0x1f] = data;

	if((cg_reg_index & 0x1f) == 0x08) //accessing VS LO reg clears VS HI reg
		cg_reg[0x09] = 0;

	if((cg_reg_index & 0x1f) == 0x0a) //accessing VE LO reg clears VE HI reg
		cg_reg[0x0b] = 0;

	if((cg_reg_index & 0x1f) == 0x05 && (cg_reg[0x05] & 0xc0) == 0x80) //clear bitmap buffer
	{
		UINT32 i;
		UINT8 *vram = memory_region(space->machine, "maincpu");

		/* TODO: this isn't yet 100% accurate */
		if(cg_reg[0x05] & 1)
		{
			for(i=0;i<0x4000;i++)
				vram[i+0x40000] = 0x00; //clear B
		}
		if(cg_reg[0x05] & 2)
		{
			for(i=0;i<0x4000;i++)
				vram[i+0x44000] = 0x00; //clear R
		}
		if(cg_reg[0x05] & 4)
		{
			for(i=0;i<0x4000;i++)
				vram[i+0x48000] = 0x00; //clear G
		}
		if(cg_reg[0x05] & 8)
		{
			for(i=0;i<0x4000;i++)
				vram[i+0x4c000] = 0x00; //clear I
		}
	}

	if(cg_reg_index & 0x80) //enable auto-inc
		cg_reg_index = (cg_reg_index & 0xfc) | ((cg_reg_index + 1) & 0x03);

	{
		static UINT16 vs,ve,hs,he;
		rectangle visarea;
		static int x_size,y_size;

		x_size = ((cg_reg[0x0e] & 0x1f) == 0x17 || (cg_reg[0x0e] & 0x1f) == 0x03) ? 640 : 320;
		y_size = ((cg_reg[0x0e] & 0x1f) == 0x03) ? 400 : 200;

		visarea.min_x = 0;
		visarea.min_y = 0;
		visarea.max_x = x_size - 1;
		visarea.max_y = y_size - 1;

		vs = (cg_reg[0x08]) | ((cg_reg[0x09]<<8) & 1);
		ve = (cg_reg[0x0a]) | ((cg_reg[0x0b]<<8) & 1);
		hs = (cg_reg[0x0c] & 0x7f)*8;
		he = (cg_reg[0x0d] & 0x7f)*8;

//		popmessage("%d %d %d %d %02x",vs,ve,hs,he,cg_reg[0x0e]);

		space->machine->primary_screen->configure(720, 480, visarea, space->machine->primary_screen->frame_period().attoseconds); //TODO
	}
}

static WRITE8_HANDLER( timer_w )
{
	running_device *pit8253 = space->machine->device("pit");

	pit8253_gate0_w(pit8253, 1);
	pit8253_gate1_w(pit8253, 1);
	pit8253_gate0_w(pit8253, 0);
	pit8253_gate1_w(pit8253, 0);
	pit8253_gate0_w(pit8253, 1);
	pit8253_gate1_w(pit8253, 1);
}

static UINT8 joy_mode;

static READ8_HANDLER( mz2500_joystick_r )
{
	static UINT8 res,dir_en,in_r;

	res = 0xff;
	in_r = ~input_port_read(space->machine,"JOY");

	if(joy_mode & 0x40)
	{
		if(!(joy_mode & 0x04)) res &= ~0x20;
		if(!(joy_mode & 0x08)) res &= ~0x10;
		dir_en = (joy_mode & 0x20) ? 0 : 1;
	}
	else
	{
		if(!(joy_mode & 0x01)) res &= ~0x20;
		if(!(joy_mode & 0x02)) res &= ~0x10;
		dir_en = (joy_mode & 0x10) ? 0 : 1;
	}

	if(dir_en)
		res &= (~((in_r) & 0x0f));

	res &= (~((in_r) & 0x30));

	return res;
}

static WRITE8_HANDLER( mz2500_joystick_w )
{
	joy_mode = data;
}


static ADDRESS_MAP_START(mz2500_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
//  AM_RANGE(0x60, 0x63) AM_WRITE(w3100a_w)
//  AM_RANGE(0x63, 0x63) AM_READ(w3100a_r)
//  AM_RANGE(0xa0, 0xa3) AM_READWRITE(sio_r,sio_w)
//  AM_RANGE(0xa4, 0xa5) AM_READWRITE(sasi_r, sasi_w)
	AM_RANGE(0xa8, 0xa8) AM_WRITE(mz2500_rom_w)
	AM_RANGE(0xa9, 0xa9) AM_READ(mz2500_rom_r)
//  AM_RANGE(0xac, 0xad) AM_WRITE(emm_w)
//  AM_RANGE(0xad, 0xad) AM_READ(emm_r)
	AM_RANGE(0xae, 0xae) AM_WRITE(palette4096_io_w)
//  AM_RANGE(0xb0, 0xb3) AM_READWRITE(sio_r,sio_w)
	AM_RANGE(0xb4, 0xb4) AM_READWRITE(mz2500_bank_addr_r,mz2500_bank_addr_w)
	AM_RANGE(0xb5, 0xb5) AM_READWRITE(mz2500_bank_data_r,mz2500_bank_data_w)
//  AM_RANGE(0xb8, 0xb9) AM_READWRITE(kanji_r,kanji_w)
	AM_RANGE(0xbc, 0xbc) AM_WRITE(mz2500_cg_addr_w)
	AM_RANGE(0xbd, 0xbd) AM_READ(mz2500_rplane_latch_r) AM_WRITE(mz2500_cg_data_w)
//	AM_RANGE(0xbc, 0xbf) AM_READ(mz2500_crtc_cg_r) //reads plane
	AM_RANGE(0xc6, 0xc6) AM_WRITE(mz2500_irq_sel_w)
	AM_RANGE(0xc7, 0xc7) AM_WRITE(mz2500_irq_data_w)
	AM_RANGE(0xc8, 0xc9) AM_DEVREADWRITE("ym", ym2203_r, ym2203_w)
//  AM_RANGE(0xca, 0xca) AM_READWRITE(voice_r,voice_w)
//  AM_RANGE(0xcc, 0xcc) AM_READWRITE(calendar_r,calendar_w)
//  AM_RANGE(0xce, 0xce) AM_WRITE(mz2500_dictionary_bank_w)
	AM_RANGE(0xcf, 0xcf) AM_WRITE(mz2500_kanji_bank_w)
	AM_RANGE(0xd8, 0xdb) AM_DEVREADWRITE("mb8877a", mz2500_wd17xx_r, mz2500_wd17xx_w)
	AM_RANGE(0xdc, 0xdd) AM_WRITE(mz2500_fdc_w)
	AM_RANGE(0xe0, 0xe3) AM_DEVREADWRITE("i8255_0", i8255a_r, i8255a_w)
    AM_RANGE(0xe4, 0xe7) AM_DEVREADWRITE("pit", pit8253_r, pit8253_w)
//	AM_RANGE(0xe8, 0xe9) AM_DEVREADWRITE("z80pio_1", z80pio_c_r, z80pio_c_w)
//	AM_RANGE(0xea, 0xeb) AM_DEVREADWRITE("z80pio_1", z80pio_d_r, z80pio_d_w)
	AM_RANGE(0xe8, 0xeb) AM_DEVREADWRITE("z80pio_1", z80pio_ba_cd_r, z80pio_ba_cd_w)
	AM_RANGE(0xef, 0xef) AM_READWRITE(mz2500_joystick_r,mz2500_joystick_w)
    AM_RANGE(0xf0, 0xf3) AM_WRITE(timer_w)
	AM_RANGE(0xf4, 0xf7) AM_READ(mz2500_crtc_hvblank_r) AM_WRITE(mz2500_tv_crtc_w)
//  AM_RANGE(0xf8, 0xf9) AM_READWRITE(extrom_r,extrom_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mz2500 )
	PORT_START("KEY0")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6)
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7)
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8)

	PORT_START("KEY1")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F9") PORT_CODE(KEYCODE_F9)
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F10") PORT_CODE(KEYCODE_F10)
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("8 (PAD)") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("9 (PAD)") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(", (PAD)") //PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(". (PAD)") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("+ (PAD)") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("- (PAD)") PORT_CODE(KEYCODE_MINUS_PAD)

	PORT_START("KEY2")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("0 (PAD)") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("1 (PAD)") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("2 (PAD)") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("3 (PAD)") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("4 (PAD)") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("5 (PAD)") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("6 (PAD)") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("7 (PAD)") PORT_CODE(KEYCODE_7_PAD)

	PORT_START("KEY3")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(27)
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("BREAK") //PORT_CODE(KEYCODE_ESC)

	PORT_START("KEY4")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START("KEY5")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START("KEY6")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START("KEY7")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("^")
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) //Yen symbol
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("_")
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(".")
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(",")

	PORT_START("KEY8")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')

	PORT_START("KEY9")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(":")
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME(";")
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("-")
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("@")
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("[")
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_UNUSED)

	PORT_START("KEYA")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("]")
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("COPY")
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("CLR")
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("INST")
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("BS")
	PORT_BIT(0x20,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("ESC")
	PORT_BIT(0x40,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("* (PAD)") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x80,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("/ (PAD)") PORT_CODE(KEYCODE_SLASH_PAD)

	PORT_START("KEYB")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("GRPH")
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("SLOCK")
	PORT_BIT(0x04,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("SHIFT")
	PORT_BIT(0x08,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("KANA")
	PORT_BIT(0x10,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("CTRL")
	PORT_BIT(0xe0,IP_ACTIVE_LOW,IPT_UNUSED)

	PORT_START("KEYC")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("KJ1")
	PORT_BIT(0x02,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("KJ2")
	PORT_BIT(0xfc,IP_ACTIVE_LOW,IPT_UNUSED)

	PORT_START("KEYD")
	PORT_BIT(0x01,IP_ACTIVE_LOW,IPT_KEYBOARD) PORT_NAME("LOGO KEY")
	PORT_BIT(0xfe,IP_ACTIVE_LOW,IPT_UNUSED)

	PORT_START("UNUSED")
	PORT_BIT(0xff,IP_ACTIVE_LOW,IPT_UNUSED )

	PORT_START("DSW0")
	PORT_DIPNAME( 0x01, 0x00, "DSW0" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	/* this enables HD-loader */
	PORT_START("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "DSW1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START("JOY")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static void mz2500_reset(UINT8 type)
{
	int i;

	for(i=0;i<8;i++)
		bank_val[i] = bank_reset_val[type][i];
}

static MACHINE_RESET(mz2500)
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	UINT8 *IPL = memory_region(machine, "ipl");
	UINT32 i;

	mz2500_reset(IPL_RESET);

	//irq_vector[0] = 0xef; /* RST 28h - vblank */

	text_col_size = 0;
	text_font_reg = 0;

	/* copy IPL to its natural bank ROM/RAM position */
	for(i=0;i<0x8000;i++)
	{
		//RAM[i] = IPL[i];
		RAM[i+0x68000] = IPL[i];
	}

	/* clear CG RAM */
	for(i=0;i<0x20000;i++)
		RAM[i+0x40000] = 0x00;

	/* disable IRQ */
	for(i=0;i<4;i++)
		irq_mask[i] = 0;
}

static const gfx_layout mz2500_cg_layout =
{
	8, 8,		/* 8 x 8 graphics */
	RGN_FRAC(1,1),		/* 512 codes */
	1,		/* 1 bit per pixel */
	{ 0 },		/* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8		/* code takes 8 times 8 bits */
};

/* gfx1 is mostly 16x16, but there are some 8x8 characters */
static const gfx_layout mz2500_8_layout =
{
	8, 8,		/* 8 x 8 graphics */
	1920,		/* 1920 codes */
	1,		/* 1 bit per pixel */
	{ 0 },		/* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8		/* code takes 8 times 8 bits */
};

static const gfx_layout mz2500_16_layout =
{
	16, 16,		/* 16 x 16 graphics */
	RGN_FRAC(1,1),		/* 8192 codes */
	1,		/* 1 bit per pixel */
	{ 0 },		/* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 128, 129, 130, 131, 132, 133, 134, 135 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16 * 16		/* code takes 16 times 16 bits */
};

static const gfx_layout mz2500_pcg_layout_1bpp =
{
	8, 8,
	RGN_FRAC(1,4),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8
};

static const gfx_layout mz2500_pcg_layout_3bpp =
{
	8, 8,
	RGN_FRAC(1,4),
	3,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8
};


static GFXDECODE_START( mz2500 )
	GFXDECODE_ENTRY("kanji", 0, mz2500_cg_layout, 0, 256)
	GFXDECODE_ENTRY("kanji", 0x4400, mz2500_8_layout, 0, 256)	// for viewer only
	GFXDECODE_ENTRY("kanji", 0, mz2500_16_layout, 0, 256)		// for viewer only
	GFXDECODE_ENTRY("pcg", 0, mz2500_pcg_layout_1bpp, 0, 1)
	GFXDECODE_ENTRY("pcg", 0, mz2500_pcg_layout_3bpp, 0, 1)
GFXDECODE_END

static INTERRUPT_GEN( mz2500_vbl )
{
	if(irq_mask[0])
		cpu_set_input_line_and_vector(device, 0, HOLD_LINE, irq_vector[0]);
}

static READ8_DEVICE_HANDLER( mz2500_porta_r )
{
	return 0xff;
}

static READ8_DEVICE_HANDLER( mz2500_portb_r )
{
	return 0xff;
}

static READ8_DEVICE_HANDLER( mz2500_portc_r )
{
	return 0xff;
}

static WRITE8_DEVICE_HANDLER( mz2500_porta_w )
{
}

static WRITE8_DEVICE_HANDLER( mz2500_portb_w )
{
}

static WRITE8_DEVICE_HANDLER( mz2500_portc_w )
{
	static UINT8 reset_lines;

	/* work RAM reset */
	if((reset_lines & 0x02) == 0x00 && (data & 0x02))
	{
		mz2500_reset(WRAM_RESET);
		/* correct? */
		cputag_set_input_line(device->machine, "maincpu", INPUT_LINE_RESET, PULSE_LINE);
	}

	/* bit 2 is speaker */

	/* IPL reset */
	if((reset_lines & 0x08) == 0x00 && (data & 0x08))
		mz2500_reset(IPL_RESET);

	reset_lines = data;

	//printf("%02x\n",data);
}

static I8255A_INTERFACE( ppi8255_intf )
{
	DEVCB_HANDLER(mz2500_porta_r),						/* Port A read */
	DEVCB_HANDLER(mz2500_portb_r),						/* Port B read */
	DEVCB_HANDLER(mz2500_portc_r),						/* Port C read */
	DEVCB_HANDLER(mz2500_porta_w),						/* Port A write */
	DEVCB_HANDLER(mz2500_portb_w),						/* Port B write */
	DEVCB_HANDLER(mz2500_portc_w)						/* Port C write */
};

static WRITE8_DEVICE_HANDLER( mz2500_pio1_porta_w )
{
//  printf("%02x\n",data);
	text_col_size = (data & 0x20) ? 1 : 0;
	key_mux = data & 0x1f;
}

static UINT8 pio_latchb;

static READ8_DEVICE_HANDLER( mz2500_pio1_porta_r )
{
	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3",
	                                        "KEY4", "KEY5", "KEY6", "KEY7",
	                                        "KEY8", "KEY9", "KEYA", "KEYB",
	                                        "KEYC", "KEYD", "UNUSED", "UNUSED" };

	if(((key_mux & 0x10) == 0x00) || ((key_mux & 0x0f) == 0x0f)) //status read
	{
		int res,i;

		res = 0xff;
		for(i=0;i<0xe;i++)
			res &= input_port_read(device->machine, keynames[i]);

		pio_latchb = res;

		return res;
	}

	pio_latchb = input_port_read(device->machine, keynames[key_mux & 0xf]);

	return input_port_read(device->machine, keynames[key_mux & 0xf]);
}

#if 0
static READ8_DEVICE_HANDLER( mz2500_pio1_portb_r )
{
	return pio_latchb;
}
#endif

static Z80PIO_INTERFACE( mz2500_pio1_intf )
{
	DEVCB_NULL,
	DEVCB_HANDLER( mz2500_pio1_porta_r ),
	DEVCB_HANDLER( mz2500_pio1_porta_w ),
	DEVCB_NULL,
	DEVCB_HANDLER( mz2500_pio1_porta_r ),
	DEVCB_NULL,
	DEVCB_NULL
};

static WRITE8_DEVICE_HANDLER( opn_porta_w )
{
	/*
	---- x--- mouse select
	---- -x-- palette bit (16/4096 colors)
	---- --x- floppy reverse bit (controls wd17xx bits in command registers)
	*/

//	printf("%02x\n",data);
	fdc_reverse = (data & 2) ? 0x00 : 0xff;
	pal_select = (data & 4) ? 1 : 0;
	if((data & 4) == 0)
		printf("Warning: 4096 color mode used\n");
}

static const ym2203_interface ym2203_interface_1 =
{
	{
		AY8910_LEGACY_OUTPUT,
		AY8910_DEFAULT_LOADS,
		DEVCB_INPUT_PORT("DSW0"),	// read A
		DEVCB_INPUT_PORT("DSW1"),	// read B
		DEVCB_HANDLER(opn_porta_w),	// write A
		DEVCB_NULL					// write B
	},
	NULL
};

static PALETTE_INIT( mz2500 )
{
	int i;

	/*set up 16 colors (TODO) */
	for(i=0;i<8;i++)
		palette_set_color_rgb(machine, i,pal1bit((i & 2)>>1),pal1bit((i & 4)>>2),pal1bit((i & 1)>>0));

	/* set up 256 colors (TODO) */

	/* set up 4096 colors */
	// ...
}

/* PIT8253 Interface */

static WRITE_LINE_DEVICE_HANDLER( pit8253_clk1_irq )
{
	if(irq_mask[1])
		cputag_set_input_line_and_vector(device->machine, "maincpu", 0, HOLD_LINE,irq_vector[1]);
}

static const struct pit8253_config mz2500_pit8253_intf =
{
	{
		{
			31250,
			DEVCB_NULL,
			DEVCB_LINE(pit8253_clk1_irq)
		},
		{
			0,
			DEVCB_NULL,
			DEVCB_LINE(pit8253_clk2_w)
		},
		{
			0,
			DEVCB_NULL,
			DEVCB_NULL
		}
	}
};

static MACHINE_DRIVER_START( mz2500 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, 6000000)
    MDRV_CPU_PROGRAM_MAP(mz2500_map)
    MDRV_CPU_IO_MAP(mz2500_io)
	MDRV_CPU_VBLANK_INT("screen", mz2500_vbl)

    MDRV_MACHINE_RESET(mz2500)

	MDRV_I8255A_ADD( "i8255_0", ppi8255_intf )
	MDRV_Z80PIO_ADD( "z80pio_1", 6000000, mz2500_pio1_intf )

	MDRV_MB8877_ADD("mb8877a",mz2500_mb8877a_interface)
	MDRV_FLOPPY_4_DRIVES_ADD(mz2500_floppy_config)


	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	//MDRV_SCREEN_RAW_PARAMS(XTAL_17_73447MHz/2, 568, 0, 320, 312, 0, 200)
	MDRV_SCREEN_RAW_PARAMS(XTAL_17_73447MHz, 640+108, 0, 320, 480, 0, 200) //TODO: fix this
	MDRV_PALETTE_LENGTH(0x200+4096) // TODO: it needs more than this
	MDRV_PALETTE_INIT(mz2500)

	MDRV_GFXDECODE(mz2500)

    MDRV_VIDEO_START(mz2500)
    MDRV_VIDEO_UPDATE(mz2500)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("ym", YM2203, 6000000/2) //unknown clock / divider
	MDRV_SOUND_CONFIG(ym2203_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)

	MDRV_PIT8253_ADD("pit", mz2500_pit8253_intf)
MACHINE_DRIVER_END



/* ROM definition */
ROM_START( mz2500 )
	ROM_REGION( 0x80000, "maincpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x08000, "ipl", 0 )
	ROM_LOAD( "ipl.rom", 0x00000, 0x8000, CRC(7a659f20) SHA1(ccb3cfdf461feea9db8d8d3a8815f7e345d274f7) )

	/* this is probably an hand made ROM, will be removed in the end ...*/
	ROM_REGION( 0x1000, "cgrom", 0 )
	ROM_LOAD( "cg.rom", 0x0000, 0x0800, CRC(a082326f) SHA1(dfa1a797b2159838d078650801c7291fa746ad81) )

	ROM_REGION( 0x40000, "kanji", 0 )
	ROM_LOAD( "kanji.rom", 0x0000, 0x40000, CRC(dd426767) SHA1(cc8fae0cd1736bc11c110e1c84d3f620c5e35b80) )

	ROM_REGION( 0x40000, "dictionary", 0 )
	ROM_LOAD( "dict.rom", 0x00000, 0x40000, CRC(aa957c2b) SHA1(19a5ba85055f048a84ed4e8d471aaff70fcf0374) )

	ROM_REGION( 0x2000, "pcg", ROMREGION_ERASEFF )

	ROM_REGION( 0x8000, "rom", ROMREGION_ERASEFF )
	ROM_LOAD( "file.rom", 0x00000, 0x8000, CRC(a7bf39ce) SHA1(3f4a237fc4f34bac6fe2bbda4ce4d16d42400081) ) //optional?
ROM_END

/* Driver */

COMP( 1985, mz2500,   0,        0,      mz2500,   mz2500,        0,      "Sharp",     "MZ-2500", GAME_NOT_WORKING | GAME_NO_SOUND)
