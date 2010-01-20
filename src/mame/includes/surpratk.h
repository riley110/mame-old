/*************************************************************************

    Surprise Attack

*************************************************************************/

typedef struct _surpratk_state surpratk_state;
struct _surpratk_state
{
	/* memory pointers */
	UINT8 *    ram;
//  UINT8 *    paletteram;    // currently this uses generic palette handling

	/* video-related */
	int        layer_colorbase[3], sprite_colorbase;
	int        layerpri[3];

	/* misc */
	int        videobank;

	/* devices */
	const device_config *maincpu;
	const device_config *k052109;
	const device_config *k053244;
	const device_config *k053251;
};

/*----------- defined in video/surpratk.c -----------*/

extern void surpratk_tile_callback(running_machine *machine, int layer,int bank,int *code,int *color,int *flags,int *priority);
extern void surpratk_sprite_callback(running_machine *machine, int *code,int *color,int *priority_mask);

VIDEO_UPDATE( surpratk );
