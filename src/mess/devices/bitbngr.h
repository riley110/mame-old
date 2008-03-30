/*********************************************************************

	bitbngr.h

	TRS style "bitbanger" serial port

*********************************************************************/

#ifndef BITBNGR_H
#define BITBNGR_H

#include "device.h"
#include "image.h"


struct bitbanger_config
{
	/* filter function; returns non-zero if input accepted */
	int (*filter)(const device_config *img, const int *pulses, int total_pulses, int total_duration);
	double pulse_threshhold;			/* the maximum duration pulse that we will consider */
	double pulse_tolerance;				/* deviation tolerance for pulses */
	int minimum_pulses;					/* the minimum amount of pulses before we start analyzing */
	int maximum_pulses;					/* the maximum amount of pulses that we will track */
	int begin_pulse_value;				/* the begining value of a pulse */
	int initial_value;					/* the initial value of the bitbanger line */
};

#define IO_BITBANGER IO_PRINTER

enum
{
	MESS_DEVINFO_PTR_BITBANGER_CONFIG = MESS_DEVINFO_PTR_DEV_SPECIFIC
};

void bitbanger_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
void bitbanger_output(const device_config *img, int value);


#endif /* BITBNGR_H */
