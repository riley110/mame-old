#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/generic.h"
#include "video/cdp1861.h"

static mame_timer *cdp1861_int_timer;
static mame_timer *cdp1861_efx_timer;
static mame_timer *cdp1861_dma_timer;

typedef struct
{
	int disp;
	int dmaout;
	int dmaptr;
	UINT8 data[CDP1861_VISIBLE_LINES * 8];
} CDP1861_VIDEO_CONFIG;

static CDP1861_VIDEO_CONFIG cdp1861;

int cdp1861_efx;

static void cdp1861_int_tick(int ref)
{
	int scanline = video_screen_get_vpos(0);

	if (scanline == CDP1861_SCANLINE_INT_START)
	{
		if (cdp1861.disp)
		{
			cpunum_set_input_line(0, CDP1802_INPUT_LINE_INT, HOLD_LINE);
		}

		mame_timer_adjust(cdp1861_int_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_INT_END, 0), 0, time_zero);
	}
	else
	{
		if (cdp1861.disp)
		{
			cpunum_set_input_line(0, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
		}

		mame_timer_adjust(cdp1861_int_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_INT_START, 0), 0, time_zero);
	}
}

static void cdp1861_efx_tick(int ref)
{
	int scanline = video_screen_get_vpos(0);

	switch (scanline)
	{
	case CDP1861_SCANLINE_EFX_TOP_START:
		cdp1861_efx = ASSERT_LINE;
		mame_timer_adjust(cdp1861_efx_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_EFX_TOP_END, 0), 0, time_zero);
		break;

	case CDP1861_SCANLINE_EFX_TOP_END:
		cdp1861_efx = CLEAR_LINE;
		mame_timer_adjust(cdp1861_efx_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_EFX_BOTTOM_START, 0), 0, time_zero);
		break;

	case CDP1861_SCANLINE_EFX_BOTTOM_START:
		cdp1861_efx = ASSERT_LINE;
		mame_timer_adjust(cdp1861_efx_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_EFX_BOTTOM_END, 0), 0, time_zero);
		break;

	case CDP1861_SCANLINE_EFX_BOTTOM_END:
		cdp1861_efx = CLEAR_LINE;
		mame_timer_adjust(cdp1861_efx_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_EFX_TOP_START, 0), 0, time_zero);
		break;
	}
}

static void cdp1861_dma_tick(int ref)
{
	if (cdp1861.dmaout)
	{
		cpunum_set_input_line(0, CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);
		mame_timer_adjust(cdp1861_dma_timer, MAME_TIME_IN_CYCLES(CDP1861_CYCLES_DMAOUT_HIGH, 0), 0, time_zero);
		cdp1861.dmaout = 0;
	}
	else
	{
		cpunum_set_input_line(0, CDP1802_INPUT_LINE_DMAOUT, HOLD_LINE);
		mame_timer_adjust(cdp1861_dma_timer, MAME_TIME_IN_CYCLES(CDP1861_CYCLES_DMAOUT_LOW, 0), 0, time_zero);
		cdp1861.dmaout = 1;
	}
}

void cdp1861_dma_w(UINT8 data)
{
	cdp1861.data[cdp1861.dmaptr] = data;
	cdp1861.dmaptr++;

	if (cdp1861.dmaptr == CDP1861_VISIBLE_LINES * 8)
	{
		mame_timer_adjust(cdp1861_dma_timer, double_to_mame_time(TIME_NEVER), 0, time_zero);
	}
}

void cdp1861_sc(int state)
{
	if (state == CDP1802_STATE_CODE_S3_INTERRUPT)
	{
		cdp1861.dmaout = 0;
		cdp1861.dmaptr = 0;

		mame_timer_adjust(cdp1861_dma_timer, MAME_TIME_IN_CYCLES(CDP1861_CYCLES_INT_DELAY, 0), 0, time_zero);
	}
}

MACHINE_RESET( cdp1861 )
{
	cdp1861_int_timer = mame_timer_alloc(cdp1861_int_tick);
	cdp1861_efx_timer = mame_timer_alloc(cdp1861_efx_tick);
	cdp1861_dma_timer = mame_timer_alloc(cdp1861_dma_tick);

	mame_timer_adjust(cdp1861_int_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_INT_START, 0), 0, time_zero);
	mame_timer_adjust(cdp1861_efx_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_EFX_TOP_START, 0), 0, time_zero);
	mame_timer_adjust(cdp1861_dma_timer, double_to_mame_time(TIME_NEVER), 0, time_zero);

	cdp1861.disp = 0;
	cdp1861.dmaout = 0;

	cdp1861_efx = CLEAR_LINE;
}

READ8_HANDLER( cdp1861_dispon_r )
{
	cdp1861.disp = 1;

	return 0xff;
}

WRITE8_HANDLER( cdp1861_dispoff_w )
{
	cdp1861.disp = 0;
}

VIDEO_START( cdp1861 )
{
	state_save_register_global(cdp1861.disp);
	state_save_register_global(cdp1861.dmaout);
	state_save_register_global(cdp1861.dmaptr);
	state_save_register_global_array(cdp1861.data);

	return 0;
}

VIDEO_UPDATE( cdp1861 )
{
	fillbitmap(bitmap, get_black_pen(Machine), cliprect);

	if (cdp1861.disp)
	{
		int i, bit;

		for (i = 0; i < CDP1861_VISIBLE_LINES * 8; i++)
		{
			int x = (i % 8) * 8;
			int y = i / 8;

			UINT8 data = cdp1861.data[i];

			for (bit = 0; bit < 8; bit++)
			{
				int color = (data & 0x80) >> 7;
				plot_pixel(bitmap, CDP1861_SCREEN_START + x + bit, CDP1861_SCANLINE_DISPLAY_START + y, Machine->pens[color]);
				data <<= 1;
			}
		}
	}

	return 0;
}
