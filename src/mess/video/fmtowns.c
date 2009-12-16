
/*
 * FM Towns video hardware
 *
 * Resolution: from 320x200 to 768x512
 *
 * Up to two graphics layers
 *
 * Sprites
 *
 * CRTC registers (16-bit):
 *
 *  0:  HSync width 1
 *  1:  HSync width 2
 *  4:  HSync total
 *  5:  VSync width 1
 *  6:  VSync width 2
 *  7:  Equalising pulse accountable time (what?)
 *  8:  VSync total
 *
 *  9:
 *  10: Graphic layer 0 horizontal start/end
 *  11:
 *  12: Graphic layer 1 horizontal start/end
 *
 *  13:
 *  14: Graphic layer 0 vertical start/end
 *  15:
 *  16: Graphic layer 1 vertical start/end
 *
 *  17: Graphic layer 0 initial address?
 *  18: Graphic layer 0 horizontal adjust
 *  19: Graphic layer 0 field indirect address offset
 *  20: Graphic layer 0 line indirect address offset
 *
 *  21-24: As above, but for Graphic layer 1
 *
 *  27: Layer zoom.     bit 0 = x2 horizontal zoom layer 0
 *  to be confirmed     bit 5 = x2 vertical zoom layer 0
 *                      bit 9 = x2 horizontal zoom layer 1
 *                      bit 13 = x2 vertical zoom layer 1
 *
 *  28: Control register 0
 *      VSync enable (bit 15) (blank display?)
 *      Scroll type (layer 0 = bit 4, layer 1 = bit 5)
 *          0 = spherical scroll, 1 = cylindrical scroll
 *
 *  29: Control register 1
 *      Dot clock (bits 1 and 0)
 *      0x00 = 28.6363MHz
 *      0x01 = 24.5454MHz
 *      0x02 = 25.175MHz
 *      0x03 = 21.0525MHz (default?)
 *
 *  30: Dummy register
 *
 *  31: Control register 2
 *
 *  Video registers:
 *
 *  0:  Graphic layer(s) type: (others likely exist)
 *      bit 4 = 2 layers
 *      bits 2-3 = layer 1 mode
 *      bits 0-1 = layer 0 mode
 *          mode: 1 = 16 colours, 2 = 256 colours, 3 = highcolour (16-bit)
 *                0 = disabled?
 *
 *  1:  Layer reverse (priority?) (bit 0)
 *      YM (bit 2) - unknown
 *      peltype (bits 4 and 5)
 *
 */

#include "driver.h"
#include "machine/pic8259.h"
#include "devices/messram.h"
#include "includes/fmtowns.h"

//#define CRTC_REG_DISP 1

static UINT8 towns_vram_wplane;
static UINT8 towns_vram_rplane;
static UINT8 towns_vram_page_sel;
static UINT8 towns_palette_select;
static UINT8 towns_palette_r[256];
static UINT8 towns_palette_g[256];
static UINT8 towns_palette_b[256];
static UINT8 towns_degipal[8];
static UINT8 towns_dpmd_flag;
static UINT8 towns_crtc_mix;
static UINT8 towns_crtc_sel;  // selected CRTC register
static UINT16 towns_crtc_reg[32];
static UINT8 towns_video_sel;  // selected video register
static UINT8 towns_video_reg[2];
static UINT8 towns_tvram_enable;
static UINT16 towns_kanji_offset;
static UINT8 towns_kanji_code_h;
static UINT8 towns_kanji_code_l;
static rectangle towns_crtc_layerscr[2];  // each layer has independent sizes

extern UINT32* towns_vram;
extern UINT8* towns_gfxvram;
extern UINT8* towns_txtvram;

extern UINT32 towns_mainmem_enable;
extern UINT32 towns_ankcg_enable;

void towns_crtc_refresh_mode(running_machine* machine)
{
	rectangle scr;
	unsigned int width,height;

	scr.min_x = scr.min_y = 0;
	scr.max_x = towns_crtc_reg[4];
	scr.max_y = towns_crtc_reg[8] / 2;

	// layer 0
	width = towns_crtc_reg[10] - towns_crtc_reg[9];
	height = (towns_crtc_reg[14] - towns_crtc_reg[13]) / 2;
	towns_crtc_layerscr[0].min_x = (scr.max_x / 2) - (width / 2);
	towns_crtc_layerscr[0].min_y = (scr.max_y / 2) - (height / 2);
	towns_crtc_layerscr[0].max_x = (scr.max_x / 2) + (width / 2);
	towns_crtc_layerscr[0].max_y = (scr.max_y / 2) + (height / 2);

	// layer 1
	width = towns_crtc_reg[12] - towns_crtc_reg[11];
	height = (towns_crtc_reg[16] - towns_crtc_reg[15]) / 2;
	towns_crtc_layerscr[1].min_x = (scr.max_x / 2) - (width / 2);
	towns_crtc_layerscr[1].min_y = (scr.max_y / 2) - (height / 2);
	towns_crtc_layerscr[1].max_x = (scr.max_x / 2) + (width / 2);
	towns_crtc_layerscr[1].max_y = (scr.max_y / 2) + (height / 2);

	// sanity checks
	if(scr.max_x == 0 || scr.max_y == 0)
		return;
	if(scr.max_x <= scr.min_x || scr.max_y <= scr.min_y)
		return;

	video_screen_configure(machine->primary_screen,scr.max_x+1,scr.max_y+1,&scr,HZ_TO_ATTOSECONDS(60));
}

READ8_HANDLER( towns_gfx_high_r )
{
	return towns_gfxvram[offset];
}

WRITE8_HANDLER( towns_gfx_high_w )
{
	towns_gfxvram[offset] = data;
}

READ8_HANDLER( towns_gfx_r )
{
	if(towns_mainmem_enable != 0)
		return messram_get_ptr(devtag_get_device(space->machine, "messram"))[offset+0xc0000];

	if(towns_vram_page_sel != 0)
		offset += 0x20000;

	return 0;
}

WRITE8_HANDLER( towns_gfx_w )
{
	if(towns_mainmem_enable != 0)
	{
		messram_get_ptr(devtag_get_device(space->machine, "messram"))[offset+0xc0000] = data;
		return;
	}
	offset = offset << 2;
	if(towns_vram_page_sel != 0)
		offset += 0x20000;
	if(towns_vram_wplane & 0x08)
	{
		towns_gfxvram[offset] &= ~0x88;
		towns_gfxvram[offset] |= ((data & 0x80) >> 4) | ((data & 0x40) << 1);
		towns_gfxvram[offset + 1] &= ~0x88;
		towns_gfxvram[offset + 1] |= ((data & 0x20) >> 2) | ((data & 0x10) << 3);
		towns_gfxvram[offset + 2] &= ~0x88;
		towns_gfxvram[offset + 2] |= ((data & 0x08)) | ((data & 0x04) << 5);
		towns_gfxvram[offset + 3] &= ~0x88;
		towns_gfxvram[offset + 3] |= ((data & 0x02) << 2) | ((data & 0x01) << 7);
	}
	if(towns_vram_wplane & 0x04)
	{
		towns_gfxvram[offset] &= ~0x44;
		towns_gfxvram[offset] |= ((data & 0x80) >> 5) | ((data & 0x40));
		towns_gfxvram[offset + 1] &= ~0x44;
		towns_gfxvram[offset + 1] |= ((data & 0x20) >> 3) | ((data & 0x10) << 2);
		towns_gfxvram[offset + 2] &= ~0x44;
		towns_gfxvram[offset + 2] |= ((data & 0x08) >> 1) | ((data & 0x04) << 4);
		towns_gfxvram[offset + 3] &= ~0x44;
		towns_gfxvram[offset + 3] |= ((data & 0x02) << 1) | ((data & 0x01) << 6);
	}
	if(towns_vram_wplane & 0x02)
	{
		towns_gfxvram[offset] &= ~0x22;
		towns_gfxvram[offset] |= ((data & 0x80) >> 6) | ((data & 0x40) >> 1);
		towns_gfxvram[offset + 1] &= ~0x22;
		towns_gfxvram[offset + 1] |= ((data & 0x20) >> 4) | ((data & 0x10) << 1);
		towns_gfxvram[offset + 2] &= ~0x22;
		towns_gfxvram[offset + 2] |= ((data & 0x08) >> 2) | ((data & 0x04) << 3);
		towns_gfxvram[offset + 3] &= ~0x22;
		towns_gfxvram[offset + 3] |= ((data & 0x02)) | ((data & 0x01) << 5);
	}
	if(towns_vram_wplane & 0x01)
	{
		towns_gfxvram[offset] &= ~0x11;
		towns_gfxvram[offset] |= ((data & 0x80) >> 7) | ((data & 0x40) >> 2);
		towns_gfxvram[offset + 1] &= ~0x11;
		towns_gfxvram[offset + 1] |= ((data & 0x20) >> 5) | ((data & 0x10));
		towns_gfxvram[offset + 2] &= ~0x11;
		towns_gfxvram[offset + 2] |= ((data & 0x08) >> 3) | ((data & 0x04) << 2);
		towns_gfxvram[offset + 3] &= ~0x11;
		towns_gfxvram[offset + 3] |= ((data & 0x02) >> 1) | ((data & 0x01) << 4);
	}
}

READ8_HANDLER( towns_video_cff80_r )
{
	UINT8* ROM = memory_region(space->machine,"user");
	if(towns_mainmem_enable != 0)
		return messram_get_ptr(devtag_get_device(space->machine, "messram"))[offset+0xcff80];

	switch(offset)
	{
		case 0x00:  // mix register
			return towns_crtc_mix;
		case 0x01:  // read/write plane select (bit 0-3 write, bit 6-7 read)
			return ((towns_vram_rplane << 6) & 0xc0) | towns_vram_wplane;
		case 0x03:  // VRAM page select (bit 5)
			if(towns_vram_page_sel != 0)
				return 0x10;
			else
				return 0x00;
		case 0x16:  // Kanji character data
			return ROM[(towns_kanji_offset << 1) + 0x180000];
		case 0x17:  // Kanji character data
			return ROM[(towns_kanji_offset++ << 1) + 0x180001];
		case 0x19:  // ANK CG ROM
			if(towns_ankcg_enable != 0)
				return 0x01;
			else
				return 0x00;
		default:
			logerror("VGA: read from invalid or unimplemented memory-mapped port %05x\n",0xcff80+offset*4);
	}

	return 0;
}

WRITE8_HANDLER( towns_video_cff80_w )
{
	if(towns_mainmem_enable != 0)
	{
		messram_get_ptr(devtag_get_device(space->machine, "messram"))[offset+0xcff80] = data;
		return;
	}

	switch(offset)
	{
		case 0x00:  // mix register
			towns_crtc_mix = data;
			break;
		case 0x01:  // read/write plane select (bit 0-3 write, bit 6-7 read)
			towns_vram_wplane = data & 0x0f;
			towns_vram_rplane = (data & 0xc0) >> 6;
			towns_update_video_banks(space);
			logerror("VGA: VRAM wplane select = 0x%02x\n",towns_vram_wplane);
			break;
		case 0x03:  // VRAM page select (bit 5)
			towns_vram_page_sel = data & 0x10;
			break;
		case 0x14:  // Kanji offset (high)
			towns_kanji_code_h = data & 0x7f;
			break;
		case 0x15:  // Kanji offset (low)
			towns_kanji_code_l = data & 0x7f;
			// this is a little over the top...
			if(towns_kanji_code_h < 0x30)
			{
				towns_kanji_offset = ((towns_kanji_code_l & 0x1f) << 4)
				                   | (((towns_kanji_code_l - 0x20) & 0x20) << 8)
				                   | (((towns_kanji_code_l - 0x20) & 0x40) << 6)
				                   | ((towns_kanji_code_h & 0x07) << 9);
			}
			else if(towns_kanji_code_h < 0x70)
			{
				towns_kanji_offset = ((towns_kanji_code_l & 0x1f) << 4)
				                   + (((towns_kanji_code_l - 0x20) & 0x60) << 8)
				                   + ((towns_kanji_code_h & 0x0f) << 9)
				                   + (((towns_kanji_code_h - 0x30) & 0x70) * 0xc00)
				                   + 0x8000;
			}
			else
			{
				towns_kanji_offset = ((towns_kanji_code_l & 0x1f) << 4)
				                   | (((towns_kanji_code_l - 0x20) & 0x20) << 8)
				                   | (((towns_kanji_code_l - 0x20) & 0x40) << 6)
				                   | ((towns_kanji_code_h & 0x07) << 9)
				                   | 0x38000;
			}
			break;
		case 0x19:  // ANK CG ROM
			towns_ankcg_enable = data & 0x01;
			towns_update_video_banks(space);
			break;
		default:
			logerror("VGA: write %08x to invalid or unimplemented memory-mapped port %05x\n",data,0xcff80+offset);
	}
}

/*
 *  port 0x440-0x443 - CRTC
 *      0x440 = register select
 *      0x442/3 = register data (16-bit)
 *      0x448 = shifter register select
 *      0x44a = shifter register data (8-bit)
 *
 */
READ8_HANDLER(towns_video_440_r)
{
	switch(offset)
	{
		case 0x00:
			return towns_crtc_sel;
		case 0x02:
			logerror("CRTC: reading register %i (0x442) [%04x]\n",towns_crtc_sel,towns_crtc_reg[towns_crtc_sel]);
			return towns_crtc_reg[towns_crtc_sel] & 0x00ff;
		case 0x03:
			logerror("CRTC: reading register %i (0x443) [%04x]\n",towns_crtc_sel,towns_crtc_reg[towns_crtc_sel]);
			return (towns_crtc_reg[towns_crtc_sel] & 0xff00) >> 8;
		case 0x08:
			return towns_video_sel;
		case 0x0a:
			logerror("Video: reading register %i (0x44a) [%02x]\n",towns_video_sel,towns_video_reg[towns_video_sel]);
			return towns_video_reg[towns_video_sel];
		case 0x0c:
			if(towns_dpmd_flag != 0)
			{
				towns_dpmd_flag = 0;
				return 0x80;
			}
			else
				return 0x00;
		default:
			logerror("VID: read port %04x\n",offset+0x440);
	}
	return 0x00;
}

WRITE8_HANDLER(towns_video_440_w)
{
	switch(offset)
	{
		case 0x00:
			towns_crtc_sel = data;
			break;
		case 0x02:
			logerror("CRTC: writing register %i (0x442) [%02x]\n",towns_crtc_sel,data);
			towns_crtc_reg[towns_crtc_sel] =
				(towns_crtc_reg[towns_crtc_sel] & 0xff00) | data;
			towns_crtc_refresh_mode(space->machine);
			break;
		case 0x03:
			logerror("CRTC: writing register %i (0x443) [%02x]\n",towns_crtc_sel,data);
			towns_crtc_reg[towns_crtc_sel] =
				(towns_crtc_reg[towns_crtc_sel] & 0x00ff) | (data << 8);
			towns_crtc_refresh_mode(space->machine);
			break;
		case 0x08:
			towns_video_sel = data & 0x01;
			break;
		case 0x0a:
			logerror("Video: writing register %i (0x44a) [%02x]\n",towns_video_sel,data);
			towns_video_reg[towns_video_sel] = data;
			break;
		default:
			logerror("VID: wrote 0x%02x to port %04x\n",data,offset+0x440);
	}
}

READ8_HANDLER(towns_video_5c8_r)
{
	logerror("VID: read port %04x\n",offset+0x5c8);
	switch(offset)
	{
		case 0x00:  // 0x5c8 - disable TVRAM?
		if(towns_tvram_enable != 0)
		{
			//towns_tvram_enable = 0;
			return 0x80;
		}
		else
			return 0x00;
	}
	return 0x00;
}

WRITE8_HANDLER(towns_video_5c8_w)
{
	const device_config* dev = devtag_get_device(space->machine,"pic8259_slave");
	switch(offset)
	{
		case 0x02:  // 0x5ca - VSync clear?
			pic8259_set_irq_line(dev,3,0);
			break;
	}
	logerror("VID: wrote 0x%02x to port %04x\n",data,offset+0x5c8);
}

/* Video/CRTC
 *
 * 0xfd90 - palette colour select
 * 0xfd92/4/6 - BRG value
 * 0xfd98-9f  - degipal(?)
 */
READ8_HANDLER(towns_video_fd90_r)
{
	switch(offset)
	{
		case 0x00:
			return towns_palette_select;
		case 0x02:
			return towns_palette_b[towns_palette_select];
		case 0x04:
			return towns_palette_r[towns_palette_select];
		case 0x06:
			return towns_palette_g[towns_palette_select];
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			return towns_degipal[offset-0x08];
	}
//  logerror("VID: read port %04x\n",offset+0xfd90);
	return 0x00;
}

WRITE8_HANDLER(towns_video_fd90_w)
{
	switch(offset)
	{
		case 0x00:
			towns_palette_select = data;
			break;
		case 0x02:
			towns_palette_b[towns_palette_select] = data;
			break;
		case 0x04:
			towns_palette_r[towns_palette_select] = data;
			break;
		case 0x06:
			towns_palette_g[towns_palette_select] = data;
			break;
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			towns_degipal[offset-0x08] = data;
			towns_dpmd_flag = 1;
			break;
	}
	logerror("VID: wrote 0x%02x to port %04x\n",data,offset+0xfd90);
}

READ8_HANDLER(towns_video_ff81_r)
{
	return ((towns_vram_rplane << 6) & 0xc0) | towns_vram_wplane;
}

WRITE8_HANDLER(towns_video_ff81_w)
{
	towns_vram_wplane = data & 0x0f;
	towns_vram_rplane = (data & 0xc0) >> 6;
	towns_update_video_banks(space);
	logerror("VGA: VRAM wplane select (I/O) = 0x%02x\n",towns_vram_wplane);
}

/*
 *  Sprite RAM, low memory
 *  Writing to 0xc8xxx or 0xcaxxx activates TVRAM mode
 *  Writing to I/O port 0x5c8 disables TVRAM mode
 *     (bit 7 returns high if TVRAM mode was previously active)
 *
 *  In TVRAM mode:
 *    0xc8000-0xc8fff: ASCII text (2 bytes each: ISO646 code, then attribute)
 *    0xca000-0xcafff: JIS code
 */
READ8_HANDLER(towns_spriteram_low_r)
{
	UINT8* RAM = messram_get_ptr(devtag_get_device(space->machine, "messram"));
	UINT8* ROM = memory_region(space->machine,"user");

	if(offset < 0x1000)
	{  // 0xc8000-0xc8fff
		if(towns_mainmem_enable == 0)
			return towns_txtvram[offset];
		else
			return RAM[offset + 0xc8000];
	}
	if(offset >= 0x1000 && offset < 0x2000)
	{  // 0xc9000-0xc9fff
		return RAM[offset + 0xc9000];
	}
	if(offset >= 0x2000 && offset < 0x3000)
	{  // 0xca000-0xcafff
		if(towns_mainmem_enable == 0)
		{
			if(towns_ankcg_enable != 0 && offset < 0x2800)
				return ROM[0x180000 + 0x3d000 + (offset-0x2000)];
			return towns_txtvram[offset];
		}
		else
			return RAM[offset + 0xca000];
	}
	return 0x00;
}

WRITE8_HANDLER(towns_spriteram_low_w)
{
	UINT8* RAM = messram_get_ptr(devtag_get_device(space->machine, "messram"));
	if(offset < 0x1000)
	{  // 0xc8000-0xc8fff
		towns_tvram_enable = 1;
		if(towns_mainmem_enable == 0)
			towns_txtvram[offset] = data;
		else
			RAM[offset + 0xc8000] = data;
	}
	if(offset >= 0x1000 && offset < 0x2000)
	{
		RAM[offset + 0xc9000] = data;
	}
	if(offset >= 0x2000 && offset < 0x3000)
	{  // 0xca000-0xcafff
		towns_tvram_enable = 1;
		if(towns_mainmem_enable == 0)
			towns_txtvram[offset] = data;
		else
			RAM[offset + 0xca000] = data;
	}
}

void towns_crtc_draw_scan_layer_hicolour(bitmap_t* bitmap,const rectangle* rect,int layer,int line,int scanline)
{
	int off = 0;
	int x;
	UINT16 colour;
	int hzoom = 1;

	if(towns_vram_page_sel != 0)
		off = 0x20000;
	if(layer != 0)
	{
		if(!(towns_video_reg[0] & 0x10))
			return;
		off += 0x40000;
		off += towns_crtc_reg[21];  // initial offset
		if(towns_crtc_reg[27] & 0x0100)
			hzoom = 2;
	}
	else
	{
		off += towns_crtc_reg[17];  // initial offset
		if(towns_crtc_reg[27] & 0x0001)
			hzoom = 2;
	}

	off += (line * (512 / hzoom) * 2);

	if(hzoom == 1)
	{
		for(x=rect->min_x;x<rect->max_x;x++)
		{
			colour = (towns_gfxvram[off] << 8) | towns_gfxvram[off+1];
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x) =
					((colour & 0xf800))
					| ((colour & 0x07c0) << 13)
					| ((colour & 0x003e) << 2);
			}
			off+=2;
		}
	}
	else
	{  // x2 horizontal zoom
		for(x=rect->min_x;x<rect->max_x;x+=2)
		{
			colour = (towns_gfxvram[off+1] << 8) | towns_gfxvram[off];
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x) =
					((colour & 0x001f) << 3)
					| ((colour & 0x7c00) << 1)
					| ((colour & 0x03e0) << 14);
				*BITMAP_ADDR32(bitmap,scanline,x+1) =
					((colour & 0x001f) << 3)
					| ((colour & 0x7c00) << 1)
					| ((colour & 0x03e0) << 14);
			}
			off+=2;
		}
	}
}

void towns_crtc_draw_scan_layer_256(bitmap_t* bitmap,const rectangle* rect,int layer,int line,int scanline)
{
	int off = 0;
	int x;
	UINT8 colour;
	int hzoom = 1;

	if(towns_vram_page_sel != 0)
		off = 0x20000;
	if(layer != 0)
	{
		if(!(towns_video_reg[0] & 0x10))
			return;
		off += 0x40000;
		off += towns_crtc_reg[21];  // initial offset
		if(towns_crtc_reg[27] & 0x0100)
			hzoom = 2;
	}
	else
	{
		off += towns_crtc_reg[17];  // initial offset
		if(towns_crtc_reg[27] & 0x0001)
			hzoom = 2;
	}

	off += (line * (512 / hzoom));

	if(hzoom == 1)
	{
		for(x=rect->min_x;x<rect->max_x;x++)
		{
			colour = towns_gfxvram[off];
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x) =
					(towns_palette_r[colour] << 16)
					| (towns_palette_g[colour] << 8)
					| (towns_palette_b[colour]);
			}
			off++;
		}
	}
	else
	{  // x2 horizontal zoom
		for(x=rect->min_x;x<rect->max_x;x+=2)
		{
			colour = towns_gfxvram[off];
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x) =
					(towns_palette_r[colour] << 16)
					| (towns_palette_g[colour] << 8)
					| (towns_palette_b[colour]);
				*BITMAP_ADDR32(bitmap,scanline,x+1) =
					(towns_palette_r[colour] << 16)
					| (towns_palette_g[colour] << 8)
					| (towns_palette_b[colour]);
			}
			off++;
		}
	}
}

void towns_crtc_draw_scan_layer_16(bitmap_t* bitmap,const rectangle* rect,int layer,int line,int scanline)
{
	int off = 0;
	int x;
	UINT8 colour;
	int hzoom = 1;

	if(towns_vram_page_sel != 0)
		off = 0x20000;
	if(layer != 0)
	{
		if(!(towns_video_reg[0] & 0x10))
			return;
		off += 0x40000;
		off += towns_crtc_reg[21];  // initial offset
		if(towns_crtc_reg[27] & 0x0100)
			hzoom = 2;
	}
	else
	{
		off += towns_crtc_reg[17];  // initial offset
		if(towns_crtc_reg[27] & 0x0001)
			hzoom = 2;
	}

	off += (line * ((rect->max_x - rect->min_x) / 2));

	if(hzoom == 1)
	{
		for(x=rect->min_x;x<rect->max_x;x+=2)
		{
			colour = towns_gfxvram[off] >> 4;
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x+1) =
					(towns_palette_r[colour] << 16)
					| (towns_palette_g[colour] << 8)
					| (towns_palette_b[colour]);
			}
			colour = towns_gfxvram[off] & 0x0f;
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x) =
					(towns_palette_r[colour] << 16)
					| (towns_palette_g[colour] << 8)
					| (towns_palette_b[colour]);
			}
			off++;
		}
	}
	else
	{  // x2 horizontal zoom
		for(x=rect->min_x;x<rect->max_x;x+=4)
		{
			colour = towns_gfxvram[off] >> 4;
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x+2) =
					(towns_palette_r[colour] << 16)
					| (towns_palette_g[colour] << 8)
					| (towns_palette_b[colour]);
				*BITMAP_ADDR32(bitmap,scanline,x+3) =
					(towns_palette_r[colour] << 16)
					| (towns_palette_g[colour] << 8)
					| (towns_palette_b[colour]);
			}
			colour = towns_gfxvram[off] & 0x0f;
			if(colour != 0)
			{
				*BITMAP_ADDR32(bitmap,scanline,x) =
					(towns_palette_r[colour] << 16)
					| (towns_palette_g[colour] << 8)
					| (towns_palette_b[colour]);
				*BITMAP_ADDR32(bitmap,scanline,x+1) =
					(towns_palette_r[colour] << 16)
					| (towns_palette_g[colour] << 8)
					| (towns_palette_b[colour]);
			}
			off++;
		}
	}
}

void towns_crtc_draw_layer(bitmap_t* bitmap,const rectangle* rect,int layer)
{
	int line;
	int scanline;

	if(layer == 0)
	{
		scanline = rect->min_y;
		switch(towns_video_reg[0] & 0x03)
		{
			case 0x01:
				for(line=0;line<(rect->max_y - rect->min_y);line++)
				{
					towns_crtc_draw_scan_layer_16(bitmap,rect,layer,line,scanline);
					scanline++;
					if(towns_crtc_reg[27] & 0x0010)  // vertical zoom
					{
						towns_crtc_draw_scan_layer_16(bitmap,rect,layer,line,scanline);
						scanline++;
					}
				}
				break;
			case 0x02:
				for(line=0;line<(rect->max_y - rect->min_y);line++)
				{
					towns_crtc_draw_scan_layer_256(bitmap,rect,layer,line,scanline);
					scanline++;
					if(towns_crtc_reg[27] & 0x0010)  // vertical zoom
					{
						towns_crtc_draw_scan_layer_256(bitmap,rect,layer,line,scanline);
						scanline++;
					}
				}
				break;
			case 0x03:
				for(line=0;line<(rect->max_y - rect->min_y);line++)
				{
					towns_crtc_draw_scan_layer_hicolour(bitmap,rect,layer,line,scanline);
					scanline++;
					if(towns_crtc_reg[27] & 0x0010)  // vertical zoom
					{
						towns_crtc_draw_scan_layer_hicolour(bitmap,rect,layer,line,scanline);
						scanline++;
					}
				}
				break;
		}
	}
	else
	{
		scanline = rect->min_y;
		switch(towns_video_reg[0] & 0x0c)
		{
			case 0x04:
				for(line=rect->min_y;line<rect->max_y;line++)
				{
					towns_crtc_draw_scan_layer_16(bitmap,rect,layer,line,scanline);
					scanline++;
					if(towns_crtc_reg[27] & 0x1000)  // vertical zoom
					{
						towns_crtc_draw_scan_layer_16(bitmap,rect,layer,line,scanline);
						scanline++;
					}
				}
				break;
			case 0x08:
				// 256 colour - TODO
				break;
			case 0x0c:
				// Highcolour - TODO
				break;
		}
	}
}

VIDEO_START( towns )
{
	towns_vram_wplane = 0x00;
}

VIDEO_UPDATE( towns )
{
	int x,y;
	UINT8 colour;
	UINT32 colourcode;
	UINT8 row;  // JIS row number
	UINT16 code;
	int off = 0;

	bitmap_fill(bitmap,cliprect,0x00000000);

	if(towns_video_reg[1] & 0x01)
	{
		towns_crtc_draw_layer(bitmap,&towns_crtc_layerscr[1],1);
		towns_crtc_draw_layer(bitmap,&towns_crtc_layerscr[0],0);
	}
	else
	{
		towns_crtc_draw_layer(bitmap,&towns_crtc_layerscr[0],0);
		towns_crtc_draw_layer(bitmap,&towns_crtc_layerscr[1],1);
	}


	// draw text layer
/*
 *  Text layer format
 *  2 bytes per character at both 0xc8000 and 0xca000
 *  0xc8xxx: Byte 1: ASCII character
 *           Byte 2: Attributes
 *             bits 2-0: GRB (or is it BRG?)
 *             bit 3: Inverse
 *             bit 4: Blink
 *             bit 5: high brightness
 *             bits 7-6: Kanji high/low
 *
 *  If either bits 6 or 7 are high, then a fullwidth Kanji character is displayed
 *  at this location.  The character displayed is represented by a 2-byte
 *  JIS code at the same offset at 0xca000.
 */
	if(towns_tvram_enable != 0)
	{
		for(y=0;y<40;y++)
		{
			off = 160 * y;
			for(x=0;x<80;x++)
			{
				colour = towns_txtvram[off+1] & 0x07;
				if(towns_txtvram[off+1] & 0x20)
				{  // Bright
					colourcode = (((colour & 0x02) ? 0xfe : 0x00) << 16)
						| (((colour & 0x04) ? 0xfe : 0x00) << 8)
						| ((colour & 0x01) ? 0xfe : 0x00);
				}
				else
				{
					colourcode = (((colour & 0x02) ? 0x7f : 0x00) << 16)
						| (((colour & 0x04) ? 0x7f : 0x00) << 8)
						| ((colour & 0x01) ? 0x7f : 0x00);
				}

				if((towns_txtvram[off+1] & 0xc0) != 0)
				{	// double width
					if(towns_txtvram[off+1] & 0x40)
					{
						row = (towns_txtvram[off+0x2000] - 32);
						code = 0;
						if(row <= 8)
						{
							code = (row & 0x07) * 0x20;
							code += ((towns_txtvram[off+0x2001]-32) & 0x1f)
									| (((towns_txtvram[off+0x2001]-32) & 0x40) << 2)
									| (((towns_txtvram[off+0x2001]-32) & 0x20) << 4);
						}
						if(row >= 16)  // Kanji
						{
							code = 0x400;
							code += ((towns_txtvram[off+0x2001]) & 0x1f)
									+ (((towns_txtvram[off+0x2001]-32) & 0x60) << 4)
									+ ((row & 0x0f) << 5)
									+ (((row-16) & 0x70) * 0x60);
						}
						drawgfx_transpen_raw(bitmap,cliprect,screen->machine->gfx[1],code,
							colourcode,	0,0,towns_crtc_layerscr[0].min_x+(x*8),towns_crtc_layerscr[0].min_y+(y*16),0);
					}
				}
				else
				{
					drawgfx_transpen_raw(bitmap,cliprect,screen->machine->gfx[0],towns_txtvram[off],
						colourcode,	0,0,towns_crtc_layerscr[0].min_x+(x*8),towns_crtc_layerscr[0].min_y+(y*16),0);
				}
				off += 2;
			}
		}
	}

#ifdef CRTC_REG_DISP
	popmessage("CRTC: %i %i %i %i %i %i %i %i %i\n%i %i %i %i | %i %i %i %i\nZOOM: %04x\nShifter: %02x %02x\nText=%i",
		towns_crtc_reg[0],towns_crtc_reg[1],towns_crtc_reg[2],towns_crtc_reg[3],
		towns_crtc_reg[4],towns_crtc_reg[5],towns_crtc_reg[6],towns_crtc_reg[7],
		towns_crtc_reg[8],
		towns_crtc_reg[9],towns_crtc_reg[10],towns_crtc_reg[11],towns_crtc_reg[12],
		towns_crtc_reg[13],towns_crtc_reg[14],towns_crtc_reg[15],towns_crtc_reg[16],
		towns_crtc_reg[27],towns_video_reg[0],towns_video_reg[1],towns_tvram_enable);
#endif

    return 0;
}

