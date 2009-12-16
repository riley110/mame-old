/**********************************************************************

    Commodore Serial Bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

C64 SERIAL BUS


 Serial Bus Pinouts


    Pin Name    Description
     1  SRQ     Serial Service Request In
     2  GND     Ground
     3  ATN     Serial Attention In/Out
     4  CLK     Serial Clock In/Out
     5  DATA    Serial Data In/Out
     6  RESET   Serial Reset

    All signals are active low.


  SRQ: Serial Service Request In

 This signal is not used on the C64. On C128 it is replaced with Fast Serial
 Clock for the 1571 disk drive.


  ATN: Serial Attention In/Out

 Sending any byte with the ATN line low (sending under attention) causes it
 to be interpreted as a Bus Command for peripherals on the serial bus.

 When the C64 brings this signal LOW, all other devices start listening for
 it to transmit an address. The device addressed must respond in a preset
 period of time; otherwise, the C64 will assume that the device addressed is
 not on the bus, and will return an error in the STATUS word.

 Usually, the address byte will be followed by one to two commands for the
 device addressed, meaning the secondary address and channel on the peripheral.
 Such a command can be one of the following:

    20
    40
    60
    E0
    F0


  CLK: Serial Clock In/Out

  This signal is for timing the data sent on the serial bus. This signal is
  always generated by the active TALKER. RISING EDGE OF THE CLOCK means data
  bit is valid.


  DATA: Serial Data In/Out

  Data on the serial bus is transmitted bit by bit at a time on this line.


  RESET: Serial Reset

  You may disconnect this line to save your disk drive. The easiest way is to
  do that on the cable, thus avoiding any modifications on your peripherals.



  Serial Bus Timing

 ___
 CLK    |____|~~~~| Ts Bit Set-up time
    : Ts : Tv : Tv Bit Valid time



     |<--------- Byte sent under attention (to devices) ------------>|

 ___    ____                                                        _____ _____
 ATN       |________________________________________________________|
       :                                :
 ___    ______     ________ ___ ___ ___ ___ ___ ___ ___ ___         :
 CLK       : |_____|      |_| |_| |_| |_| |_| |_| |_| |_| |______________ _____
       :       :        :                 :         :
       : Tat : :Th: Tne :                             : Tf : Tr :
 ____   ________ : :  :___________________________________:____:
 DATA   ___|\\\\\\\\\\__:__|    |__||__||__||__||__||__||__||__|    |_________ _____
                  :     0   1   2   3   4   5   6   7      :
                  :    LSB                         MSB     :
              :     :                      :
              :     : Data Valid      Listener: Data Accepted
              : Listener READY-FOR-DATA




        END-OR-IDENTIFY HANDSHAKE (LAST BYTE IN MESSAGE)
 ___    _______________________________________________________________________
 ATN
 ___     ___ ___      ________________ ___ ___ ___ ___ ___ ___ ___ ___       __
 CLK    _| |_| |______|              |_| |_| |_| |_| |_| |_| |_| |_| |_______|_
           :      :          :                               :       :
           :Tf:Tbb:Th:Tye:Tei:Try:                               :Tf :Tfr:
 ____   __________:   :  :___:   :_______________________________________:   :_
 DATA   |__||__|  |______|   |___|   :                                   |___|_
     6   7        :  :   :   :   :                   :
        MSB       :  :   :   :   : Talker Sending            :
              :  :   :   : Listener READY-FOR-DATA      System
              :  :   : EOI-Timeout Handshake          Line Release
              :  : Listener READY-FOR-DATA
              : Talker Ready-To-Send




        TALK-ATTENTION TURN AROUND (TALKER AND LISTENER REVERSED)
 ___                 _________________________________________________________
 ATN    _____________|
             :
 ___     ___ ___     :   _____   ________ ___ ___ ___ ___ ___ ___ ___ ___
 CLK    _| |_| |_________|   |___|      |_| |_| |_| |_| |_| |_| |_| |_| |_____
           :     :   :   :   :      :                               :
           :Tf:Tr:Ttk:Tdc:Tda:Th:Try:                               :Tf :
 ____   __________:  :       :   :  :_______________________________________:
 DATA   |__||__|  |_________________|   :|__||__||__||__||__||__||__||__|   |_
     6   7       :   :   :   :  :   : 0   1   2   3   4   5   6   7
        MSB      :   :   :   :  :   :LSB                         MSB
             :   :   :   :  :   :
             :   :   :   :  :   : Data Valid
             :   :   :   :  : Listener READY-FOR-DATA
             :   :   :   : Talker Ready-To-Send
             :   :   : Device acknowledges it's now TALKER.
             :   : Becomes LISTENER, Clock = High, Data = Low
             : Talker Ready-To-Send




 ___    _____________________________________________________________________
 ATN
 ___        _________ ___ ___ ___ ___ ___ ___ ___ ___       ________ ___ ___
 CLK    ____|       |_| |_| |_| |_| |_| |_| |_| |_| |_______|      |_| |_| |_
        :       :                   :       :      :
        :Th :Tne:                               :Tf :Tbb:Th:Tne:
 ____       :   :___:___________________________________:      :_____________
 DATA   ________|   :|__||__||__||__||__||__||__||__|   |______|
        :   :   : 0   1   2   3   4   5   6   7     :
        :   :   :LSB                         MSB    :
        :   :   :                   :
        :   :   : TALKER SENDING        Listener: Data Accepted
        :   : LISTENER READY-FOR-DATA
        : TALKER READY-TO-SEND



  Serial Bus Timing


    Description         Symbol   Min     Typ     Max

    ATN Response (required) 1)   Tat      -   - 1000us
    Listener Hold-Off        Th   0   -  oo
    Non-EOI Response to RFD 2)   Tne      -  40us   200us
    Bit Set-Up Talker  4)        Ts  20us    70us     -
    Data Valid           Tv  20us    20us     -
    Frame Handshake  3)      Tf   0   20    1000us
    Frame to Release of ATN      Tr  20us     -   -
    Between Bytes Time       Tbb    100us     -   -
    EOI Response Time        Tye    200us   250us     -
    EOI Response Hold Time  5)   Tei     60us     -   -
    Talker Response Limit        Try      0  30us    60us
    Byte-Acknowledge  4)         Tpr     20us    30us     -
    Talk-Attention Release       Ttk     20us    30us   100us
    Talk-Attention Acknowledge   Tdc      0   -   -
    Talk-Attention Ack. Hold     Tda     80us     -   -
    EOI Acknowledge          Tfr     60us     -   -


   Notes:
    1)  If maximum time exceeded, device not present error.
    2)  If maximum time exceeded, EOI response required.
    3)  If maximum time exceeded, frame error.
    4)  Tv and Tpr minimum must be 60us for external device to be a talker.
    5)  Tei minimum must be 80us for external device to be a listener.
*/

#include "driver.h"
#include "cbmserial.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

enum
{
	SRQ = 0,
	ATN,
	CLK,
	DATA,
	RESET,
	SIGNAL_COUNT
};

static const char *const SIGNAL_NAME[] = { "SRQ", "ATN", "CLK", "DATA", "RESET" };

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cbmserial_daisy_state cbmserial_daisy_state;
struct _cbmserial_daisy_state
{
	cbmserial_daisy_state		*next;			/* next device */
	const device_config			*device;		/* associated device */

	int line[SIGNAL_COUNT];				/* serial signal states */

	cbmserial_line				out_line_func[SIGNAL_COUNT];
};

typedef struct _cbmserial_t cbmserial_t;
struct _cbmserial_t
{
	cbmserial_daisy_state *daisy_state;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE cbmserial_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == CBMSERIAL);
	return (cbmserial_t *)device->token;
}

INLINE const cbmserial_daisy_chain *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == CBMSERIAL));
	return (const cbmserial_daisy_chain *) device->static_config;
}

INLINE int get_signal(const device_config *device, int line)
{
	cbmserial_t *cbmserial = get_safe_token(device);
	cbmserial_daisy_state *daisy = cbmserial->daisy_state;
	int state = 1;

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		if (!daisy->line[line])
		{
			state = 0;
			break;
		}
	}

	return state;
}

INLINE void set_signal(const device_config *serial_bus_device, const device_config *calling_device, int line, int state)
{
	cbmserial_t *cbmserial = get_safe_token(serial_bus_device);
	cbmserial_daisy_state *daisy = cbmserial->daisy_state;
	int data = 1;

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		if (!strcmp(daisy->device->tag, calling_device->tag))
		{
			if (daisy->line[line] != state)
			{
				if (LOG) logerror("CBM Serial Bus: '%s' %s %u\n", calling_device->tag, SIGNAL_NAME[line], state);
				daisy->line[line] = state;
			}
			break;
		}
	}

	data = get_signal(serial_bus_device, line);
	daisy = cbmserial->daisy_state;

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		if (daisy->out_line_func[line]) (daisy->out_line_func[line])(daisy->device, data);
	}

	if (LOG) logerror("CBM Serial Bus: SRQ %u ATN %u CLK %u DATA %u RESET %u\n", get_signal(serial_bus_device, SRQ),get_signal(serial_bus_device, ATN),get_signal(serial_bus_device, CLK),get_signal(serial_bus_device, DATA),get_signal(serial_bus_device, RESET));
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

void cbmserial_srq_w(const device_config *serial_bus_device, const device_config *calling_device, int state)
{
	set_signal(serial_bus_device, calling_device, SRQ, state);
}

READ_LINE_DEVICE_HANDLER( cbmserial_srq_r )
{
	return get_signal(device, SRQ);
}

void cbmserial_atn_w(const device_config *serial_bus_device, const device_config *calling_device, int state)
{
	set_signal(serial_bus_device, calling_device, ATN, state);
}

READ_LINE_DEVICE_HANDLER( cbmserial_atn_r )
{
	return get_signal(device, ATN);
}

void cbmserial_clk_w(const device_config *serial_bus_device, const device_config *calling_device, int state)
{
	set_signal(serial_bus_device, calling_device, CLK, state);
}

READ_LINE_DEVICE_HANDLER( cbmserial_clk_r )
{
	return get_signal(device, CLK);
}

void cbmserial_data_w(const device_config *serial_bus_device, const device_config *calling_device, int state)
{
	set_signal(serial_bus_device, calling_device, DATA, state);
}

READ_LINE_DEVICE_HANDLER( cbmserial_data_r )
{
	return get_signal(device, DATA);
}

void cbmserial_reset_w(const device_config *serial_bus_device, const device_config *calling_device, int state)
{
	set_signal(serial_bus_device, calling_device, RESET, state);
}

READ_LINE_DEVICE_HANDLER( cbmserial_reset_r )
{
	return get_signal(device, RESET);
}

/*-------------------------------------------------
    DEVICE_START( cbmserial )
-------------------------------------------------*/

static DEVICE_START( cbmserial )
{
	cbmserial_t *cbmserial = get_safe_token(device);
	const cbmserial_daisy_chain *daisy = get_interface(device);
	int i;

	astring *tempstring = astring_alloc();
	cbmserial_daisy_state *head = NULL;
	cbmserial_daisy_state **tailptr = &head;

	/* create a linked list of devices */
	for ( ; daisy->tag != NULL; daisy++)
	{
		*tailptr = auto_alloc(device->machine, cbmserial_daisy_state);

		(*tailptr)->next = NULL;
		(*tailptr)->device = devtag_get_device(device->machine, daisy->tag);

		if ((*tailptr)->device == NULL)
		{
			astring_free(tempstring);
			fatalerror("Unable to locate device '%s'", daisy->tag);
		}

		for (i = SRQ; i < SIGNAL_COUNT; i++)
		{
			(*tailptr)->line[i] = 1;
		}

		(*tailptr)->out_line_func[SRQ] = (cbmserial_line)device_get_info_fct((*tailptr)->device, DEVINFO_FCT_CBM_SERIAL_SRQ);
		(*tailptr)->out_line_func[ATN] = (cbmserial_line)device_get_info_fct((*tailptr)->device, DEVINFO_FCT_CBM_SERIAL_ATN);
		(*tailptr)->out_line_func[CLK] = (cbmserial_line)device_get_info_fct((*tailptr)->device, DEVINFO_FCT_CBM_SERIAL_CLK);
		(*tailptr)->out_line_func[DATA] = (cbmserial_line)device_get_info_fct((*tailptr)->device, DEVINFO_FCT_CBM_SERIAL_DATA);
		(*tailptr)->out_line_func[RESET] = (cbmserial_line)device_get_info_fct((*tailptr)->device, DEVINFO_FCT_CBM_SERIAL_RESET);

		tailptr = &(*tailptr)->next;
	}

	cbmserial->daisy_state = head;

	astring_free(tempstring);

	/* register for state saving */
//  state_save_register_device_item(device, 0, cbmserial->);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( cbmserial )
-------------------------------------------------*/

DEVICE_GET_INFO( cbmserial )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(cbmserial_t);								break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;												break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(cbmserial);					break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							/* Nothing */												break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore Serial Bus");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "IEEE-488");								break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}
