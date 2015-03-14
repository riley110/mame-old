#include "sound/dac.h"

class suna16_state : public driver_device
{
public:
	suna16_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this,"maincpu"),
		m_spriteram(*this, "spriteram"),
		m_spriteram2(*this, "spriteram2"),
		m_dac1(*this, "dac1"),
		m_dac2(*this, "dac2"),
		m_dac3(*this, "dac3"),
		m_dac4(*this, "dac4"),
		m_gfxdecode(*this, "gfxdecode"),
		m_screen(*this, "screen"),
		m_palette(*this, "palette")
	{ }

	required_device<cpu_device> m_maincpu;
	required_shared_ptr<UINT16> m_spriteram;
	optional_shared_ptr<UINT16> m_spriteram2;
	required_device<dac_device> m_dac1;
	required_device<dac_device> m_dac2;
	optional_device<dac_device> m_dac3;
	optional_device<dac_device> m_dac4;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<screen_device> m_screen;
	required_device<palette_device> m_palette;
	UINT8 m_prot;
	UINT16 *m_paletteram;
	int m_color_bank;

	DECLARE_WRITE16_MEMBER(suna16_soundlatch_w);
	DECLARE_WRITE16_MEMBER(bssoccer_leds_w);
	DECLARE_WRITE16_MEMBER(uballoon_leds_w);
	DECLARE_READ8_MEMBER(uballoon_prot_r);
	DECLARE_WRITE8_MEMBER(uballoon_prot_w);
	DECLARE_WRITE16_MEMBER(bestbest_coin_w);
	DECLARE_READ8_MEMBER(bestbest_prot_r);
	DECLARE_WRITE8_MEMBER(bestbest_prot_w);
	DECLARE_WRITE8_MEMBER(bssoccer_pcm_1_bankswitch_w);
	DECLARE_WRITE8_MEMBER(bssoccer_pcm_2_bankswitch_w);
	DECLARE_WRITE8_MEMBER(uballoon_pcm_1_bankswitch_w);
	DECLARE_WRITE16_MEMBER(suna16_flipscreen_w);
	DECLARE_WRITE16_MEMBER(bestbest_flipscreen_w);
	DECLARE_READ16_MEMBER(suna16_paletteram16_r);
	DECLARE_WRITE16_MEMBER(suna16_paletteram16_w);
    DECLARE_MACHINE_START(bssoccer);
	DECLARE_WRITE8_MEMBER(bssoccer_DAC1_w);
	DECLARE_WRITE8_MEMBER(bssoccer_DAC2_w);
	DECLARE_WRITE8_MEMBER(bssoccer_DAC3_w);
	DECLARE_WRITE8_MEMBER(bssoccer_DAC4_w);
	DECLARE_WRITE8_MEMBER(bestbest_ay8910_port_a_w);
	DECLARE_DRIVER_INIT(uballoon);
	virtual void video_start();
	DECLARE_MACHINE_RESET(uballoon);
    DECLARE_MACHINE_START(uballoon);
	UINT32 screen_update_suna16(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_bestbest(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	TIMER_DEVICE_CALLBACK_MEMBER(bssoccer_interrupt);
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect, UINT16 *sprites, int gfx);
};
