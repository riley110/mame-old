/***************************************************************************
				Taito Custom Chips
***************************************************************************/

int PC080SN_vh_start(int chips,int gfxnum,int x_offset,int y_offset,int y_invert,int opaque,int dblwidth);
void PC080SN_vh_stop(void);
READ16_HANDLER ( PC080SN_word_0_r );
WRITE16_HANDLER( PC080SN_word_0_w );
READ16_HANDLER ( PC080SN_xscroll_word_0_r );
WRITE16_HANDLER( PC080SN_xscroll_word_0_w );
READ16_HANDLER ( PC080SN_yscroll_word_0_r );
WRITE16_HANDLER( PC080SN_yscroll_word_0_w );
READ16_HANDLER ( PC080SN_ctrl_word_0_r );
WRITE16_HANDLER( PC080SN_ctrl_word_0_w );
READ16_HANDLER ( PC080SN_word_1_r );
WRITE16_HANDLER( PC080SN_word_1_w );
READ16_HANDLER ( PC080SN_xscroll_word_1_r );
WRITE16_HANDLER( PC080SN_xscroll_word_1_w );
READ16_HANDLER ( PC080SN_yscroll_word_1_r );
WRITE16_HANDLER( PC080SN_yscroll_word_1_w );
READ16_HANDLER ( PC080SN_ctrl_word_1_r );
WRITE16_HANDLER( PC080SN_ctrl_word_1_w );
void PC080SN_set_scroll(int chip,int tilemap_num,int scrollx,int scrolly);
void PC080SN_set_trans_pen(int chip,int tilemap_num,int pen);
void PC080SN_tilemap_update(void);
void PC080SN_tilemap_draw(struct osd_bitmap *bitmap,int chip,int layer,int flags,UINT32 priority);


/***************************************************************************/

int TC0100SCN_vh_start(int chips,int gfxnum,int x_offset);

/* Function to set separate color banks for the three tilemapped layers.
   To change from the default (0,0,0) use after calling TC0100SCN_vh_start */
void TC0100SCN_set_colbanks(int bg0,int bg1,int fg);

/* Function to set separate color banks for each TC0100SCN.
   To change from the default (0,0,0) use after calling TC0100SCN_vh_start */
void TC0100SCN_set_chip_colbanks(int chip0,int chip1,int chip2);

void TC0100SCN_vh_stop(void);
READ16_HANDLER ( TC0100SCN_word_0_r );
WRITE16_HANDLER( TC0100SCN_word_0_w );
READ16_HANDLER ( TC0100SCN_ctrl_word_0_r );
WRITE16_HANDLER( TC0100SCN_ctrl_word_0_w );
READ16_HANDLER ( TC0100SCN_word_1_r );
WRITE16_HANDLER( TC0100SCN_word_1_w );
READ16_HANDLER ( TC0100SCN_ctrl_word_1_r );
WRITE16_HANDLER( TC0100SCN_ctrl_word_1_w );
READ16_HANDLER ( TC0100SCN_word_2_r );
WRITE16_HANDLER( TC0100SCN_word_2_w );
READ16_HANDLER ( TC0100SCN_ctrl_word_2_r );
WRITE16_HANDLER( TC0100SCN_ctrl_word_2_w );

/* Functions to write multiple TC0100SCNs with the same data */
WRITE16_HANDLER( TC0100SCN_dual_screen_w );
WRITE16_HANDLER( TC0100SCN_triple_screen_w );

void TC0100SCN_tilemap_update(void);
void TC0100SCN_tilemap_draw(struct osd_bitmap *bitmap,int chip,int layer,int flags,UINT32 priority);

/* returns 0 or 1 depending on the lowest priority tilemap set in the internal
   register. Use this function to draw tilemaps in the correct order. */
int TC0100SCN_bottomlayer(int chip);


/***************************************************************************/

int TC0280GRD_vh_start(int gfxnum);
void TC0280GRD_vh_stop(void);
READ16_HANDLER ( TC0280GRD_word_r );
WRITE16_HANDLER( TC0280GRD_word_w );
WRITE16_HANDLER( TC0280GRD_ctrl_word_w );
void TC0280GRD_tilemap_update(int base_color);
void TC0280GRD_zoom_draw(struct osd_bitmap *bitmap,int xoffset,int yoffset,UINT32 priority);

int TC0430GRW_vh_start(int gfxnum);
void TC0430GRW_vh_stop(void);
READ16_HANDLER ( TC0430GRW_word_r );
WRITE16_HANDLER( TC0430GRW_word_w );
WRITE16_HANDLER( TC0430GRW_ctrl_word_w );
void TC0430GRW_tilemap_update(int base_color);
void TC0430GRW_zoom_draw(struct osd_bitmap *bitmap,int xoffset,int yoffset,UINT32 priority);


/***************************************************************************/

int TC0480SCP_vh_start(int gfxnum,int pixels,int x_offset,int y_offset,int text_xoffs,int text_yoffs,int col_base);
void TC0480SCP_vh_stop(void);
READ16_HANDLER ( TC0480SCP_word_r );
WRITE16_HANDLER( TC0480SCP_word_w );
READ16_HANDLER ( TC0480SCP_ctrl_word_r );
WRITE16_HANDLER( TC0480SCP_ctrl_word_w );
void TC0480SCP_tilemap_update(void);
void TC0480SCP_tilemap_draw(struct osd_bitmap *bitmap,int layer,int flags,UINT32 priority);

/* Returns the priority order of the bg tilemaps set in the internal
   register. The order in which the four layers should be drawn is
   returned in the lowest four nibbles  (msn = bottom layer; lsn = top) */
int TC0480SCP_get_bg_priority(void);


/***************************************************************************/

int TC0110PCR_vh_start(void);
int TC0110PCR_1_vh_start(void);	/* 2nd chip */
int TC0110PCR_2_vh_start(void);	/* 3rd chip */
void TC0110PCR_vh_stop(void);
void TC0110PCR_1_vh_stop(void);	/* 2nd chip */
void TC0110PCR_2_vh_stop(void);	/* 3rd chip */
READ16_HANDLER ( TC0110PCR_word_r );
READ16_HANDLER ( TC0110PCR_word_1_r );	/* 2nd chip */
READ16_HANDLER ( TC0110PCR_word_2_r );	/* 3rd chip */
WRITE16_HANDLER( TC0110PCR_word_w );	/* color index goes up in step of 2 */
WRITE16_HANDLER( TC0110PCR_step1_word_w );	/* color index goes up in step of 1 */
WRITE16_HANDLER( TC0110PCR_step1_word_1_w );	/* 2nd chip */
WRITE16_HANDLER( TC0110PCR_step1_word_2_w );	/* 3rd chip */
WRITE16_HANDLER( TC0110PCR_step1_rbswap_word_w );	/* swaps red and blue components */
WRITE16_HANDLER( TC0110PCR_step1_4bpg_word_w );	/* only 4 bits per color gun */

WRITE_HANDLER( TC0360PRI_w );
WRITE16_HANDLER( TC0360PRI_halfword_w );
WRITE16_HANDLER( TC0360PRI_halfword_swap_w );


/***************************************************************************/

READ_HANDLER ( TC0220IOC_r );
WRITE_HANDLER( TC0220IOC_w );
READ_HANDLER ( TC0220IOC_port_r );
WRITE_HANDLER( TC0220IOC_port_w );
READ_HANDLER ( TC0220IOC_portreg_r );
WRITE_HANDLER( TC0220IOC_portreg_w );

READ_HANDLER ( TC0510NIO_r );
WRITE_HANDLER( TC0510NIO_w );
