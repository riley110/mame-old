/***************************************************************************

        Vector06c driver by Miodrag Milanovic

        10/07/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "includes/vector06.h"

VIDEO_START( vector06 )
{
}

VIDEO_UPDATE( vector06 )
{
 	UINT8 code1,code2,code3,code4;
 	UINT8 col;//,col1,col2;
	int y, x, b;

	int width = (vector_video_mode==0x00) ? 256 : 512;		
	rectangle screen_area = {0,width+64-1,0,256+64-1};
	// fill border color
	fillbitmap(bitmap, vector_color_index, &screen_area);

	// draw image
	for (x = 0; x < 32; x++)
	{
		for (y = 0; y < 256; y++)
		{
			code1 = mess_ram[0x8000 + x*256 + y];
			code2 = mess_ram[0xa000 + x*256 + y];
			code3 = mess_ram[0xc000 + x*256 + y];
			code4 = mess_ram[0xe000 + x*256 + y];
			for (b = 0; b < 8; b++)
			{
				col = ((code1 >> b) & 0x01) * 8 + ((code2 >> b) & 0x01) * 4 + ((code3 >> b) & 0x01)* 2+ ((code4 >> b) & 0x01);
				if (vector_video_mode==0x00) {
					*BITMAP_ADDR16(bitmap, 255-y+32, x*8+(7-b)+32) =  col;
				} else {
					*BITMAP_ADDR16(bitmap, 255-y+32, x*16+(7-b)*2+1+32) =  ((code2 >> b) & 0x01) * 2;
					*BITMAP_ADDR16(bitmap, 255-y+32, x*16+(7-b)*2+32)   =  ((code3 >> b) & 0x01) * 2;
				}
			}
		}
	}
	return 0;
}

PALETTE_INIT( vector06 )
{
	int i;
	for(i=0;i<16;i++) {
		palette_set_color( machine, i, MAKE_RGB(0,0,0) );
	}
}
