// license:BSD-3-Clause
// copyright-holders:David Haywood, Nicola Salmoria
#include "emu.h"
#include "includes/usgames.h"


void usgames_state::usgames_palette(palette_device &palette) const
{
	for (int j = 0; j < 16; j++)
	{
		int r = BIT(j, 0);
		int g = BIT(j, 1);
		int b = BIT(j, 2);
		int const i = BIT(j, 3);

		r = 0xff * r;
		g = 0x7f * g * (i + 1);
		b = 0x7f * b * (i + 1);

		palette.set_pen_color(j, rgb_t(r, g, b));
	}
}

void usgames_state::video_start()
{
	// assumes it can make an address mask from m_videoram.length() - 1
	assert(!(m_videoram.length() & (m_videoram.length() - 1)));

	m_gfxdecode->gfx(0)->set_source(m_charram);
}

void usgames_state::charram_w(offs_t offset, uint8_t data)
{
	m_charram[offset] = data;
	m_gfxdecode->gfx(0)->mark_dirty(offset/8);
}

MC6845_UPDATE_ROW(usgames_state::update_row)
{
	uint32_t *pix = &bitmap.pix(y);
	ra &= 0x07;

	for (int x = 0; x < x_count; x++)
	{
		int tile_index = (x + ma) & ((m_videoram.length() - 1)/2);
		int tile = m_videoram[tile_index*2];
		int attr = m_videoram[tile_index*2+1];
		uint8_t bg_color = attr & 0xf;
		uint8_t fg_color = (attr & 0xf0) >> 4;

		const uint8_t plane = m_charram[(tile << 3) | ra];
		for (int n = 7; n >= 0; n--)
			*pix++ = m_palette->pen(BIT(plane, n) ? fg_color : bg_color);
	}
}

