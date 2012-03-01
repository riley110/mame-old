/*****************************************************************************
 *
 * includes/z88.h
 *
 ****************************************************************************/

#ifndef Z88_H_
#define Z88_H_

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/upd65031.h"
#include "sound/speaker.h"
#include "rendlay.h"

#define Z88_NUM_COLOURS 3

#define Z88_SCREEN_WIDTH        640
#define Z88_SCREEN_HEIGHT       64

#define Z88_SCR_HW_REV  (1<<4)
#define Z88_SCR_HW_HRS  (1<<5)
#define Z88_SCR_HW_UND  (1<<1)
#define Z88_SCR_HW_FLS  (1<<3)
#define Z88_SCR_HW_GRY  (1<<2)
#define Z88_SCR_HW_CURS (Z88_SCR_HW_HRS|Z88_SCR_HW_FLS|Z88_SCR_HW_REV)
#define Z88_SCR_HW_NULL (Z88_SCR_HW_HRS|Z88_SCR_HW_GRY|Z88_SCR_HW_REV)

class z88_state : public driver_device
{
public:
	z88_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu"),
		  m_speaker(*this, SPEAKER_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_speaker;

	virtual void machine_start();
	void bankswitch_update(int bank, UINT16 page, int rams);
	DECLARE_READ8_MEMBER(kb_r);

	// defined in video/z88.c
	inline void plot_pixel(bitmap_ind16 &bitmap, int x, int y, UINT16 color);
	inline UINT8* convert_address(UINT32 offset);
	void vh_render_8x8(bitmap_ind16 &bitmap, int x, int y, UINT16 pen0, UINT16 pen1, UINT8 *gfx);
	void vh_render_6x8(bitmap_ind16 &bitmap, int x, int y, UINT16 pen0, UINT16 pen1, UINT8 *gfx);
	void vh_render_line(bitmap_ind16 &bitmap, int x, int y, UINT16 pen);
	void lcd_update(bitmap_ind16 &bitmap, UINT16 sbf, UINT16 hires0, UINT16 hires1, UINT16 lores0, UINT16 lores1, int flash);
};


/*----------- defined in video/z88.c -----------*/

extern PALETTE_INIT( z88 );


#endif /* Z88_H_ */
