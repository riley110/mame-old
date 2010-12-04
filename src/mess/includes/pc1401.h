/*****************************************************************************
 *
 * includes/pc1401.h
 *
 * Pocket Computer 1401
 *
 ****************************************************************************/

#ifndef PC1401_H_
#define PC1401_H_

#define CONTRAST (input_port_read(machine, "DSW0") & 0x07)


class pc1401_state : public driver_device
{
public:
	pc1401_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 portc;
	UINT8 outa;
	UINT8 outb;
	int power;
	UINT8 reg[0x100];
};


/*----------- defined in machine/pc1401.c -----------*/

int pc1401_reset(running_device *device);
int pc1401_brk(running_device *device);
void pc1401_outa(running_device *device, int data);
void pc1401_outb(running_device *device, int data);
void pc1401_outc(running_device *device, int data);
int pc1401_ina(running_device *device);
int pc1401_inb(running_device *device);

DRIVER_INIT( pc1401 );
NVRAM_HANDLER( pc1401 );


/*----------- defined in video/pc1401.c -----------*/

READ8_HANDLER(pc1401_lcd_read);
WRITE8_HANDLER(pc1401_lcd_write);
VIDEO_UPDATE( pc1401 );


#endif /* PC1401_H_ */
