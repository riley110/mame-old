/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Thomson 8-bit computers

**********************************************************************/

#include "driver.h"
#include "timer.h"
#include "state.h"
#include "device.h"
#include "machine/thomson.h"
#include "video/thomson.h"
#include "machine/wd17xx.h"
#include "devices/flopdrv.h"
#include "devices/thomflop.h"
#include "formats/thom_dsk.h"
#include "machine/mc6854.h"
#include "machine/mc6843.h"

#define VERBOSE 0 /* 0, 1 or 2 */

#define PRINT(x) mame_printf_info x

#if VERBOSE > 1
#define LOG(x)	logerror x
#define VLOG(x)	logerror x
#elif VERBOSE
#define LOG(x)	logerror x
#define VLOG(x)
#else
#define LOG(x)
#define VLOG(x)
#endif




/******************* 3''1/2 & 5''1/4 disk format ********************/

/*
  single density sector format:

  data   bytes

  id field     00      6      byte synchro
  FE      1      id field mark
  1      track
  1      side
  1      sector
  00      1      log sector size (0->128, 1->256, 2->512,...)
  2      CRC (unemulated)
  FF     12      spaces
  data field   00      6      bytes synchro
  FB      1      data field mark
  E5    128      actual data (set to E5 when formatting)
  2      CRC (unemulated)
  FF     22 ?    spaces



  double density sector format:

  data   bytes

  id field     00     12      bit synchro
  A1      3      byte synchro
  FE      1      id field mark
  1      track
  1      side
  1      sector
  01      1      log sector size (0->128, 1->256, 2->512,...)
  2      CRC (unemulated)
  4E/F7   22      spaces
  data field   00     12      bit synchro
  A1      3      bytes synchro
  FB      1      data field mark
  E5    256      actual data (set to E5 when formatting)
  2      CRC (unemulated)
  4E/F7   74 ?    spaces

  => at most  392 bytes / sector
  => at most 6272 bytes / track

  Notes: 
  - the BIOS puts 4E bytes as spaces after id and data fields
  - some protected games expect F7 bytes (instead of 4E) after id fields
*/

#define THOM_SIZE_ID          32
#define THOM_SIZE_DATA_LO    (128+80)
#define THOM_SIZE_DATA_HI    (256+80)
#define THOM_SIZE_SYNCHRO     12


/* build an identifier, with header & space */
static int thom_floppy_make_addr( chrn_id id, UINT8* dst, int sector_size )
{
	if ( sector_size == 128 ) 
	{
		/* single density */
		memset( dst, 0x00, 6 ); /* synchro bytes */
		dst[  7 ] = 0xfe; /* address field mark */
		dst[  8 ] = id.C;
		dst[  9 ] = id.H;
		dst[ 10 ] = id.N;
		dst[ 11 ] = id.R;
		dst[ 12 ] = 0; /* TODO: CRC */
		dst[ 13 ] = 0; /* TODO: CRC */
		memset( dst + 14, 0xff, 12 ); /* end mark */
		return 36;
	}
	else 
	{
		/* double density */
		memset( dst, 0xa1, 3 ); /* synchro bytes */
		dst[ 3 ] = 0xfe; /* address field mark */
		dst[ 4 ] = id.C;
		dst[ 5 ] = id.H;
		dst[ 6 ] = id.N;
		dst[ 7 ] = id.R;
		dst[ 8 ] = 0; /* TODO: CRC */
		dst[ 9 ] = 0; /* TODO: CRC */
		memset( dst + 10, 0xf7, 22 ); /* end mark */
		return 32;
	}
}



/* build a sector, with header & space */
static int thom_floppy_make_sector( mess_image* img, chrn_id id, UINT8* dst, int sector_size )
{
	if ( sector_size == 128 ) 
	{
		/* single density */
		memset( dst, 0x00, 6 ); /* synchro bytes */
		dst[ 6 ] = 0xfb; /* data field mark */
		floppy_drive_read_sector_data
			( img, id.H, id.R, dst + 7, sector_size );
		dst[ sector_size + 7 ] = 0; /* TODO: CRC */
		dst[ sector_size + 8 ] = 0; /* TODO: CRC */
		memset( dst + sector_size + 9, 0xff, 22 ); /* end mark */
		return sector_size + 31;
	}
	else 
	{
		/* double density */
		memset( dst, 0xa1, 3 ); /* synchro bytes */
		dst[ 3 ] = 0xfb; /* data field mark */
		floppy_drive_read_sector_data
			( img, id.H, id.R, dst + 4, sector_size );
		dst[ sector_size + 4 ] = 0; /* TODO: CRC */
		dst[ sector_size + 5 ] = 0; /* TODO: CRC */
		memset( dst + sector_size + 6, 0xF7, 74 ); /* end mark */
		return sector_size + 80;
	}
}



/* build a whole track */
static int thom_floppy_make_track( mess_image* img, UINT8* dst, int sector_size, int side )
{
	UINT8 space = ( sector_size == 128 ) ? 0xff : 0;
	UINT8* org = dst;
	chrn_id id;
	int nb;

	/* go to start of track */
	while ( ! floppy_drive_get_flag_state( img, FLOPPY_DRIVE_INDEX ) ) 
	{
		if ( ! floppy_drive_get_next_id( img, side, &id ) ) 
			return 0;
	}

	/* for each sector... */
	for ( nb = 0; nb < 16; nb++ ) 
	{
		if ( ! floppy_drive_get_next_id( img, side, &id ) ) 
			break;

		memset( dst, space, THOM_SIZE_SYNCHRO ); dst += THOM_SIZE_SYNCHRO;
		dst += thom_floppy_make_addr( id, dst, sector_size );
		memset( dst, space, THOM_SIZE_SYNCHRO ); dst += THOM_SIZE_SYNCHRO;
		dst += thom_floppy_make_sector( img, id, dst, sector_size );

		if ( floppy_drive_get_flag_state( img, FLOPPY_DRIVE_INDEX ) ) 
			break;
	}
	return dst - org;
}



/******************* QDD disk format ********************/

/*
  sector format:

  data   bytes

  id field     16     17      synchro
  A5      1      id field mark
  1      sector (1-400) hi byte
  1      sector (1-400) low byte
  1      check-sum (sum modulo 256 of 3 last bytes)

  data field   16     10      synchro
  5A      1      data field mark
  FF    128      actual data (set to FF when formatting)
  1      check-sum (sum modulo 256 of 129 last bytes)


  there are 400 sectors numbered from 1
*/

#define THOM_QDD_SYNCH_DISK 100
#define THOM_QDD_SYNCH_ADDR  17
#define THOM_QDD_SYNCH_DATA  10

#define THOM_QDD_SIZE_ID     (   4 + THOM_QDD_SYNCH_ADDR  )
#define THOM_QDD_SIZE_DATA   ( 130 + THOM_QDD_SYNCH_DATA )


/* build an identifier, with header */
static int thom_qdd_make_addr( int sector, UINT8* dst )
{
	dst[  0 ] = 0xa5;
	dst[  1 ] = sector >> 8;
	dst[  2 ] = sector & 0xff;
	dst[  3 ] = dst[ 0 ] + dst[ 1 ] + dst[ 2 ];
	return 4;
}



/* build a sector, with header */
static int thom_qdd_make_sector( mess_image* img, int sector, UINT8* dst )
{
	int i;
	dst[ 0 ] = 0x5a;
	floppy_drive_read_sector_data ( img, 0, sector, dst + 1, 128 );
	dst[ 129 ] = 0;
	for ( i = 0; i < 129; i++ )
		dst[ 129 ] += dst[ i ];
	return 130;
}



/* build a whole disk */
static int thom_qdd_make_disk ( mess_image* img, UINT8* dst )
{
	UINT8* org = dst;
	int i;

	memset( dst, 0x16, THOM_QDD_SYNCH_DISK ); dst += THOM_QDD_SYNCH_DISK;

	for ( i = 1; i <= 400; i++ ) 
	{
		memset( dst, 0x16, THOM_QDD_SYNCH_ADDR ); dst += THOM_QDD_SYNCH_ADDR;
		dst += thom_qdd_make_addr( i, dst );
		memset( dst, 0x16, THOM_QDD_SYNCH_DATA ); dst += THOM_QDD_SYNCH_DATA;
		dst += thom_qdd_make_sector( img, i, dst );
	}

	memset( dst, 0x16, THOM_QDD_SYNCH_DISK ); dst += THOM_QDD_SYNCH_DISK;

	return dst - org;
}



/*********************** CD 90-640 controller ************************/

/* 5''1/4 two-sided double-density
   used in TO7, TO7/70, MO5 computers
   based on a WD2793 (or lower ?) chip
*/



static UINT8 to7_5p14_select;



static READ8_HANDLER ( to7_5p14_r )
{
	if ( offset < 4 ) 
		return wd17xx_r( offset );
	else if ( offset == 8 ) 
		return to7_5p14_select;
	else
		logerror ( "%f $%04x to7_5p14_r: invalid read offset %i\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), offset );
	return 0;
}



static WRITE8_HANDLER( to7_5p14_w )
{
	if ( offset < 4 ) 
		wd17xx_w( offset, data );
	else if ( offset == 8 ) 
	{
		/* drive select */
		int drive = -1, side = 0;
		DENSITY dens;
    
		switch ( data & 7 ) 
		{
		case 0: break;
		case 2: drive = 0; side = 0; break;
		case 3: drive = 1; side = 1; break;
		case 4: drive = 2; side = 0; break;
		case 5: drive = 3; side = 1; break;
		default:
			logerror( "%f $%04x to7_5p14_w: invalid drive select pattern $%02X\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data );
		}
    
		dens = (data & 0x80) ? DEN_FM_LO : DEN_MFM_LO;
		thom_floppy_set_density( dens );
		wd17xx_set_density( dens );
    
		to7_5p14_select = data;

		if ( drive != -1 ) 
		{
			thom_floppy_active( 0 );
			wd17xx_set_drive( drive );
			wd17xx_set_side( side );
			LOG(( "%f $%04x to7_5p14_w: $%02X set drive=%i side=%i density=%s\n", 
			      attotime_to_double(timer_get_time()), activecpu_get_previouspc(),
			      data, drive, side, (dens == DEN_FM_LO) ? "FM" : "MFM" ));
		}
	}
	else
		logerror ( "%f $%04x to7_5p14_w: invalid write offset %i (data=$%02X)\n", 
			   attotime_to_double(timer_get_time()), activecpu_get_previouspc(), offset, data );
}



static void to7_5p14_reset( void )
{
	int i;
	LOG(( "to7_5p14_reset: CD 90-640 controller\n" ));
	thom_floppy_set_density( DEN_MFM_LO );
	wd17xx_reset();
	for ( i = 0; i < device_count( IO_FLOPPY ); i++ ) 
	{
		mess_image * img = image_from_devtype_and_index( IO_FLOPPY, i );
		floppy_drive_set_ready_state( img, FLOPPY_DRIVE_READY, 0 );
		floppy_drive_set_rpm( img, 300. );
		floppy_drive_seek( img, - floppy_drive_get_current_track( img ) );
	}
}



static void to7_5p14_init( void )
{
	LOG(( "to7_5p14_init: CD 90-640 controller\n" ));
	wd17xx_init( WD_TYPE_2793, NULL, NULL );
	state_save_register_global( to7_5p14_select );
}



/*********************** CD 90-015 controller ************************/

/* 5''1/4 one-sided single-density (up to 4 one-sided drives)
   used in TO7, TO7/70, MO5 computers
   based on HD 46503 S chip, but we actually use a MC 6843 instead
   (seems they are clone)
*/



static UINT8 to7_5p14sd_select;



static READ8_HANDLER ( to7_5p14sd_r )
{
	if ( offset < 8 ) 
		return mc6843_r( offset );
	else if ( offset >= 8 && offset <= 9 ) 
		return to7_5p14sd_select;
	else
		logerror ( "%f $%04x to7_5p14sd_r: invalid read offset %i\n",  attotime_to_double(timer_get_time()), activecpu_get_previouspc(), offset );
	return 0;
}



static WRITE8_HANDLER( to7_5p14sd_w )
{
	if ( offset < 8 ) 
		mc6843_w( offset, data );
	else if ( offset >= 8 && offset <= 9 ) 
	{
		/* drive select */
		int drive = -1, side = 0;
    
		if ( data & 1 )      
		{ 
			drive = 0; 
			side = 0; 
		}
		else if ( data & 2 ) 
		{ 
			drive = 1; 
			side = 1; 
		}
		else if ( data & 4 ) 
		{ 
			drive = 2; 
			side = 0; 
		}
		else if ( data & 8 ) 
		{ 
			drive = 3; 
			side = 1; 
		}

		to7_5p14sd_select = data;

		if ( drive != -1 ) 
		{
			thom_floppy_active( 0 );
			mc6843_set_drive( drive );
			mc6843_set_side( side );
			LOG(( "%f $%04x to7_5p14sd_w: $%02X set drive=%i side=%i\n", 
			      attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data, drive, side ));
		}
	}
	else
		logerror ( "%f $%04x to7_5p14sd_w: invalid write offset %i (data=$%02X)\n", 
			   attotime_to_double(timer_get_time()), activecpu_get_previouspc(), offset, data );
}



static void to7_5p14sd_reset( void )
{
	int i;
	LOG(( "to7_5p14sd_reset: CD 90-015 controller\n" ));
	thom_floppy_set_density( DEN_FM_LO );
	mc6843_reset();
	for ( i = 0; i < device_count( IO_FLOPPY ); i++ ) 
	{
		mess_image * img = image_from_devtype_and_index( IO_FLOPPY, i );
		floppy_drive_set_ready_state( img, FLOPPY_DRIVE_READY, 0 );
		floppy_drive_set_rpm( img, 300. );
		floppy_drive_seek( img, - floppy_drive_get_current_track( img ) );
	}
}



static mc6843_interface to7_6843_itf = { NULL };



static void to7_5p14sd_init( void )
{
	LOG(( "to7_5p14sd_init: CD 90-015 controller\n" ));
	mc6843_config( &to7_6843_itf );
	state_save_register_global( to7_5p14sd_select );
}


/*********************** CQ 90-028 controller ************************/

/* QDD 2''8 controller
   used in TO7, TO7/70, MO5 computers
   it is based on a MC6852 SSDA

   Note: the MC6852 is only partially emulated, most features are not used in 
   the controller and are ignored.
*/



/* MC6852 status */
#define QDD_S_RDA  0x01 /* receiver data available */
#define QDD_S_TDRA 0x02 /* transitter data register available */
#define QDD_S_NDCD 0x04 /* data carrier detect, negated (unused) */
#define QDD_S_NCTS 0x08 /* clear-to-send, negated write-protect */
#define QDD_S_TUF  0x10 /* transmitter underflow (unused) */
#define QDD_S_OVR  0x20 /* receiver overrun (unused) */
#define QDD_S_PE   0x40 /* receiver parity error (unused) */
#define QDD_S_IRQ  0x80 /* interrupt request */
#define QDD_S_ERR  (QDD_S_TUF | QDD_S_OVR | QDD_S_PE | QDD_S_NDCD | QDD_S_NCTS)


/* MC6852 control */
#define QDD_C1_RIE       0x20 /* interrupt on reveive */
#define QDD_C1_TIE       0x10 /* interrupt on transmit */
#define QDD_C1_CLRSYNC   0x08 /* clear receiver sync char (unused) */
#define QDD_C1_STRIPSYNC 0x04 /* strips sync from received (unused) */
#define QDD_C1_TRESET    0x02 /* transmitter reset */
#define QDD_C1_RRESET    0x01 /* receiver reset */
#define QDD_C2_EIE       0x80 /* interrupt on error */
#define QDD_C2_TSYNC     0x40 /* underflow = ff if 0 / sync if 1 (unused) */
#define QDD_C2_BLEN      0x04 /* transfer byte length (unused) */
#define QDD_C3_CLRTUF    0x08 /* clear underflow */
#define QDD_C3_CLRCTS    0x04 /* clear CTS  */
#define QDD_C3_SYNCLEN   0x02 /* sync byte length (unused) */
#define QDD_C3_SYNCMODE  0x01 /* external / internal sync mode (unused) */


/* a track is actually the whole disk = 400 128-byte sectors + headers */
#define QDD_MAXBUF ( THOM_QDD_SIZE_ID + THOM_QDD_SIZE_DATA ) * 512


static struct 
{
	/* MC6852 registers */
	UINT8 status;
	UINT8 ctrl1;
	UINT8 ctrl2;
	UINT8 ctrl3;

	/* extra registers */
	UINT8 drive;

	/* internal state */
	UINT8  data[QDD_MAXBUF];  /* enough for a whole track */
	UINT32 data_idx;          /* byte position in track */
	UINT32 start_idx;         /* start of write position */
	UINT32 data_size;         /* track length */
	UINT8  data_crc;          /* checksum when writing */
	UINT8  index_pulse;       /* one pulse per track */

} * to7qdd;



static void to7_qdd_index_pulse_cb ( mess_image* img, int state )
{
	to7qdd->index_pulse = state;

	if ( state ) 
	{
		/* rewind to disk start */
		to7qdd->data_idx = 0;
		to7qdd->start_idx = 0;
		to7qdd->data_size = 0;
	}

	VLOG(( "%f to7_qdd_pulse_cb: state=%i\n", attotime_to_double(timer_get_time()), state ));
}



static mess_image * to7_qdd_image ( void )
{
	return image_from_devtype_and_index( IO_FLOPPY, 0 );
}



/* update MC6852 status register */
static void to7_qdd_stat_update( void )
{
	int flags = floppy_drive_get_flag_state( to7_qdd_image(), -1 );

	/* byte-ready */
	to7qdd->status |= QDD_S_RDA | QDD_S_TDRA;
	if ( ! to7qdd->drive ) 
		to7qdd->status |= QDD_S_PE;

	/* write-protect */
	if ( flags & FLOPPY_DRIVE_DISK_WRITE_PROTECTED ) 
		to7qdd->status |= QDD_S_NCTS;

	/* sticky reset conditions */
	if ( to7qdd->ctrl1 & QDD_C1_RRESET ) 
		to7qdd->status &= ~(QDD_S_PE | QDD_S_RDA | QDD_S_OVR);
	if ( to7qdd->ctrl1 & QDD_C1_TRESET ) 
		to7qdd->status &= ~(QDD_S_TDRA | QDD_S_TUF);
  
	/* irq update */

	if ( ( (to7qdd->ctrl1 & QDD_C1_RIE) && !(to7qdd->status & QDD_S_RDA ) ) ||
	     ( (to7qdd->ctrl1 & QDD_C1_TIE) && !(to7qdd->status & QDD_S_TDRA) ) ||
	     ( (to7qdd->ctrl2 & QDD_C2_EIE) && !(to7qdd->status & QDD_S_ERR ) ) )
		to7qdd->status &= ~QDD_S_IRQ;

	if ( ( (to7qdd->ctrl1 & QDD_C1_RIE) && (to7qdd->status & QDD_S_RDA ) ) ||
	     ( (to7qdd->ctrl1 & QDD_C1_TIE) && (to7qdd->status & QDD_S_TDRA) ) ||
	     ( (to7qdd->ctrl2 & QDD_C2_EIE) && (to7qdd->status & QDD_S_ERR ) ) )
		to7qdd->status |= QDD_S_IRQ;
}



static UINT8 to7_qdd_read_byte( void )
{
	UINT8 data;
  
	/* rebuild disk if needed */
	if ( !to7qdd->data_size ) 
	{
		to7qdd->data_size = thom_qdd_make_disk( to7_qdd_image(), to7qdd->data );
		assert( to7qdd->data_idx < sizeof( to7qdd->data ) );
	}
  
	if ( to7qdd->data_idx >= to7qdd->data_size ) 
		data = 0;
	else 
		data = to7qdd->data[ to7qdd->data_idx ];
  
	VLOG(( "%f $%04x to7_qdd_read_byte: RDATA off=%i/%i data=$%02X\n", 
	       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), 
	       to7qdd->data_idx, to7qdd->data_size, data ));
  
	to7qdd->data_idx++;
	to7qdd->start_idx = to7qdd->data_idx;
  
	return data;
}



/* This is quite complex: bytes are written one at a time by the CPU and we
   must detect the following patterns:
   * CPU write id field and data field => format
   * CPU write data field after it has read an id field => sector write
   */
static void to7_qdd_write_byte( UINT8 data )
{
	int i;

	/* rebuild disk if needed */
	if ( !to7qdd->data_size ) 
	{
		to7qdd->data_size = thom_qdd_make_disk( to7_qdd_image(), to7qdd->data );
		assert( to7qdd->data_idx < sizeof( to7qdd->data ) );
	}

	if ( ( to7qdd->start_idx != to7qdd->data_idx || /* field in construction */
	       data==0xA5 || data==0x5A ) &&    /* first byte of tentative field */
	     to7qdd->data_idx <to7qdd->data_size )
	{
    
		/* this is the first byte of the field */
		if ( to7qdd->start_idx == to7qdd->data_idx ) 
			to7qdd->data_crc = 0;
    
		/* accumulate bytes */
		to7qdd->data[ to7qdd->data_idx ] = data;
		to7qdd->data_idx++;
    
		VLOG (( "%f $%04x to7_qdd_write_byte: got $%02X offs=%i-%i\n", 
			attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data,
			to7qdd->start_idx, to7qdd->data_idx ));
    
		/* end of tentative id field */
		if ( to7qdd->data_idx == to7qdd->start_idx + 4 &&
		     to7qdd->data[ to7qdd->start_idx ] == 0xA5 && 
		     to7qdd->data[ to7qdd->start_idx + 3 ] == to7qdd->data_crc ) 
		{
      
			/* got an id field => format */
			int sector = (int) to7qdd->data[ to7qdd->start_idx + 1 ] * 256 + (int) to7qdd->data[ to7qdd->start_idx + 2 ];
			UINT8 filler = 0xff;
      
			LOG(( "%f $%04x to7_qdd_write_byte: got id field for sector=%i\n", 
			      attotime_to_double(timer_get_time()), activecpu_get_previouspc(), sector ));
	  
			floppy_drive_format_sector( to7_qdd_image(),
						    0, sector, 0, 0, sector, 128, filler );
			to7qdd->start_idx = to7qdd->data_idx;
		}
    
		/* end of tentative data field */
		else if ( to7qdd->data_idx == to7qdd->start_idx + 130 &&
			  to7qdd->data[ to7qdd->start_idx  ] == 0x5A && 
			  to7qdd->data[ to7qdd->start_idx + 129 ] == to7qdd->data_crc ) 
		{
      
			/* look backwards for previous id field */
			for ( i = to7qdd->start_idx - 3; i >= 0; i-- )
			{
				if ( to7qdd->data[ i ] == 0xA5 &&
				     ( ( to7qdd->data[ i ] + to7qdd->data[ i + 1 ] + 
					 to7qdd->data[ i + 2 ] ) & 0xff 
					     ) == to7qdd->data[ i + 3 ] )
					break;
			}
      
			if ( i >= 0 ) 
			{
				/* got an id & a data field => write */
				int sector = (int) to7qdd->data[ i + 1 ] * 256 + (int) to7qdd->data[ i + 2 ];
	
				LOG(( "%f $%04x to7_qdd_write_byte: goto data field for sector=%i\n", 
				      attotime_to_double(timer_get_time()), activecpu_get_previouspc(), sector ));
				
				floppy_drive_write_sector_data( to7_qdd_image(), 0, sector, to7qdd->data + to7qdd->start_idx + 1, 128, 0 );
			}
      
			to7qdd->start_idx = to7qdd->data_idx;
		}
    
		else to7qdd->data_crc += data;
	}
}



static READ8_HANDLER ( to7_qdd_r )
{
	switch ( offset ) 
	{

	case 0: /* MC6852 status */
		to7_qdd_stat_update();
		VLOG(( "%f $%04x to7_qdd_r: STAT=$%02X irq=%i pe=%i ovr=%i und=%i tr=%i rd=%i ncts=%i\n",
		       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), to7qdd->status,
		       to7qdd->status & QDD_S_IRQ  ? 1 : 0,
		       to7qdd->status & QDD_S_PE   ? 1 : 0,
		       to7qdd->status & QDD_S_OVR  ? 1 : 0,
		       to7qdd->status & QDD_S_TUF  ? 1 : 0,
		       to7qdd->status & QDD_S_TDRA ? 1 : 0,
		       to7qdd->status & QDD_S_RDA  ? 1 : 0,
		       to7qdd->status & QDD_S_NCTS ? 1 : 0 ));
		return to7qdd->status;

	case 1: /* MC6852 data input => read byte from disk */
		to7qdd->status &= ~(QDD_S_RDA | QDD_S_PE | QDD_S_OVR);
		to7_qdd_stat_update();
		return to7_qdd_read_byte();

	case 8: /* floppy status */
	{ 
		UINT8 data = 0;
		mess_image* img = to7_qdd_image();
		if ( ! image_exists( img ) ) 
			data |= 0x40; /* disk present */
		if ( to7qdd->index_pulse ) 
			data |= 0x80; /* disk start */
		VLOG(( "%f $%04x to7_qdd_r: STATUS8 $%02X\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data ));
		return data;
	}
 
	default:
		logerror ( "%f $%04x to7_qdd_r: invalid read offset %i\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), offset );
		return 0;
	}
}



static WRITE8_HANDLER( to7_qdd_w )
{
	switch ( offset ) 
	{
    
	case 0: /* MC6852 control 1 */
		/* reset */
		if ( data & QDD_C1_RRESET ) 
			to7qdd->status &= ~(QDD_S_PE | QDD_S_RDA | QDD_S_OVR);
		if ( data & QDD_C1_TRESET ) 
			to7qdd->status &= ~(QDD_S_TDRA | QDD_S_TUF);

		to7qdd->ctrl1 = ( data & ~(QDD_C1_RRESET | QDD_C1_TRESET) ) |( data &  (QDD_C1_RRESET | QDD_C1_TRESET) & to7qdd->ctrl1 );
		to7_qdd_stat_update();
		VLOG(( "%f $%04x to7_qdd_w: CTRL1=$%02X reset=%c%c %s%sirq=%c%c\n",
		       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data,
		       data & QDD_C1_RRESET ? 'r' : '-', data & QDD_C1_TRESET ? 't' : '-',
		       data & QDD_C1_STRIPSYNC ? "strip-sync " : "",
		       data & QDD_C1_CLRSYNC ? "clear-sync " : "",
		       data & QDD_C1_RIE ? 'r' : '-', 
		       data & QDD_C1_TIE ? 't' : '-' ));
		break;
    
	case 1:
		switch ( to7qdd->ctrl1 >> 6 ) 
		{
      
		case 0: /* MC6852 control 2 */
		{ 
#if 0
			/* most of these are unused now */
			static const int bit[8] = { 6, 6, 7, 8, 7, 7, 8, 8 };
			static const int par[8] = { 2, 1, 0, 0, 2, 1, 2, 1 };
			static const char* parname[3] = { "none", "odd", "even" };
			int bits, parity;
			bits   = bit[ (data >> 3) & 7 ];
			parity = par[ (data >> 3) & 7 ];
			to7_qdd_stat_update();
			VLOG(( "%f $%04x to7_qdd_w: CTRL2=$%02X bits=%i par=%s blen=%i under=%s%s\n",
			       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data,
			       bits, parname[ parity ], data & QDD_C2_BLEN ? 1 : 2,
			       data & QDD_C2_TSYNC ? "sync" : "ff",
			       data & QDD_C2_EIE ? "irq-err" : "" ));
#endif
			to7qdd->ctrl2 = data;
			break;
		}
	  
		case 1: /* MC6852 control 3 */
			to7qdd->ctrl3 = data;
			/* reset just once each write, not sticky */
			if ( data & QDD_C3_CLRTUF ) 
				to7qdd->status &= ~QDD_S_TUF;
			if ( data & QDD_C3_CLRCTS ) 
				to7qdd->status &= ~QDD_S_NCTS;
			to7_qdd_stat_update();
			VLOG(( "%f $%04x to7_qdd_w: CTRL3=$%02X %s%ssync-len=%i sync-mode=%s\n",
			       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data,
			       data & QDD_C3_CLRTUF ? "clr-tuf " : "",
			       data & QDD_C3_CLRCTS ? "clr-cts " : "",
			       data & QDD_C3_SYNCLEN ? 1 : 2,
			       data & QDD_C3_SYNCMODE ? "ext" : "int" ));
			break;

		case 2: /* MC6852 sync code => write byte to disk */
			to7_qdd_write_byte( data );
			break;

		case 3: /* MC6852 data out => does not seem to be used */
			VLOG(( "%f $%04x to7_qdd_w: ignored WDATA=$%02X\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data ));
			break;

		}
		break;

	case 8: /* set drive */
		to7qdd->drive = data;
		VLOG(( "%f $%04x to7_qdd_w: DRIVE=$%02X\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data ));
		break;

	case 12: /* motor pulse ? */
		thom_floppy_active( 0 );
		VLOG(( "%f $%04x to7_qdd_w: MOTOR=$%02X\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data ));
		break;

	default:
		logerror ( "%f $%04x to7_qdd_w: invalid write offset %i (data=$%02X)\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), offset, data );
	}
}



static void to7_qdd_reset( void )
{
	int i;
	LOG(( "to7_qdd_reset: CQ 90-028 controller\n" ));

	thom_floppy_set_density( DEN_MFM_LO );

	for ( i = 0; i < device_count( IO_FLOPPY ); i++ ) 
	{
		mess_image * img = image_from_devtype_and_index( IO_FLOPPY, i );
		floppy_drive_set_index_pulse_callback( img, to7_qdd_index_pulse_cb );
		floppy_drive_set_ready_state( img, FLOPPY_DRIVE_READY, 0 );
		floppy_drive_set_motor_state( img, 1 );
		/* pulse each time the whole-disk spiraling track ends */
		/* at 90us per byte read, the disk can be read in 6s */
		floppy_drive_set_rpm( img, 60. / 6. );
	}

	to7qdd->ctrl1 |= QDD_C1_TRESET | QDD_C1_RRESET; /* reset */
	to7qdd->ctrl2 &= 0x7c; /* clear EIE, PC2-PC1 */
	to7qdd->ctrl3 &= 0xfe; /* internal sync */
	to7qdd->drive = 0;
	to7_qdd_stat_update();
}



static void to7_qdd_init( void )
{
	LOG(( "to7_qdd_init: CQ 90-028 controller\n" ));

	to7qdd = auto_malloc( sizeof( *to7qdd ) );
	assert( to7qdd );

	state_save_register_global( to7qdd->status );
	state_save_register_global( to7qdd->ctrl1 );
	state_save_register_global( to7qdd->ctrl2 );
	state_save_register_global( to7qdd->ctrl3 );
	state_save_register_global( to7qdd->drive );
	state_save_register_global( to7qdd->data_idx );
	state_save_register_global( to7qdd->start_idx );
	state_save_register_global( to7qdd->data_size );
	state_save_register_global( to7qdd->data_crc );
	state_save_register_global( to7qdd->index_pulse );
	state_save_register_global_array( to7qdd->data );
}



/********************** THMFC1 controller *************************/

/* custom Thomson Gate-array for 3''1/2, 5''1/4 (double density only) & QDD
 */



/* types of high-level operations */
#define THMFC1_OP_RESET         0
#define THMFC1_OP_WRITE_SECT    1
#define THMFC1_OP_READ_ADDR     2
#define THMFC1_OP_READ_SECT     3


/* STAT0 flags */
#define THMFC1_STAT0_SYNCHRO        0x01 /* bit clock synchronized */
#define THMFC1_STAT0_BYTE_READY_OP  0x02 /* byte ready (high-level operation) */
#define THMFC1_STAT0_CRC_ERROR      0x04
#define THMFC1_STAT0_FINISHED       0x08
#define THMFC1_STAT0_FINISHING      0x10 /* (unemulated) */
#define THMFC1_STAT0_BYTE_READY_POL 0x80 /* polling mode */


/*#define THOM_MAXBUF (THOM_SIZE_ID+THOM_SIZE_DATA_HI+2*THOM_SIZE_SYNCHRO)*17*/
#define THOM_MAXBUF ( THOM_QDD_SIZE_ID + THOM_QDD_SIZE_DATA ) * 512


static struct 
{

	UINT8   op;
	UINT8   sector;            /* target sector, in [1,16] */
	UINT8   track;             /* current track, in [0,79] */
	UINT8   side;              /* current side, 0 or 1 */
	UINT8   drive;             /* 0 to 3 */
	UINT16  sector_size;       /* 128 or 256 (512, 1024 not supported) */
	UINT8   formatting;
	UINT8   ipl;               /* index pulse / QDD start */
	UINT8   wsync;             /* synchronization word */

	UINT8   data[THOM_MAXBUF]; /* enough for a whole track */
	UINT32  data_idx;          /* reading / writing / formatting pos */
	UINT32  data_size;         /* bytes to read / write */
	UINT32  data_finish;       /* when to raise the finished flag */
	UINT32  data_raw_idx;      /* byte index for raw track reading */
	UINT32  data_raw_size;     /* size of track already cached in data */
	UINT8   data_crc;          /* check-sum of written data */

	UINT8   stat0;             /* status register */

} * thmfc1;


static emu_timer* thmfc_floppy_cmd;



static mess_image * thmfc_floppy_image ( void )
{
	return image_from_devtype_and_index( IO_FLOPPY, thmfc1->drive );
}



static int thmfc_floppy_is_qdd ( void )
{
	return thom_floppy_get_type( thmfc1->drive & 2 ) == THOM_FLOPPY_QDD;
}



static void thmfc_floppy_index_pulse_cb ( mess_image *img, int state )
{
	if ( img != thmfc_floppy_image() ) 
		return;

	if ( thmfc_floppy_is_qdd() ) 
	{
		/* pulse each time the whole-disk spiraling track ends */
		floppy_drive_set_rpm( img, 423. / 25. );
		thmfc1->ipl = state;
		if ( state ) 
		{
			thmfc1->data_raw_size = 0;
			thmfc1->data_raw_idx = 0;
			thmfc1->data_idx = 0;
		}
	}
	else 
	{
		floppy_drive_set_rpm( img, 300. );
		thmfc1->ipl = state;
		if ( state  ) 
			thmfc1->data_raw_idx = 0;
	}

	VLOG(( "%f thmfc_floppy_index_pulse_cb: state=%i\n", attotime_to_double(timer_get_time()), state ));
}



static int thmfc_floppy_find_sector ( chrn_id* dst )
{
	mess_image* img = thmfc_floppy_image();
	chrn_id id;
	int r = 0;

	/* scan track, try 4 revolutions */
	while ( r < 4 ) 
	{

		if ( floppy_drive_get_next_id( img, thmfc1->side, &id ) ) 
		{
			if ( id.C == thmfc1->track &&
			     id.R == thmfc1->sector &&
			     (128 << id.N) == thmfc1->sector_size
			     /* check side ?  id.H == thmfc1->side */ ) 
			{
				if ( dst ) 
					memcpy( dst, &id, sizeof( chrn_id ) );
				thmfc1->stat0 = THMFC1_STAT0_BYTE_READY_POL;
				LOG (( "thmfc_floppy_find_sector: sector found C=%i H=%i R=%i N=%i\n", id.C, id.H, id.R, id.N ));
				return 1;
			}
		}

		if ( floppy_drive_get_flag_state( img, FLOPPY_DRIVE_INDEX ) ) 
			r++;
	}

	thmfc1->stat0 = THMFC1_STAT0_CRC_ERROR | THMFC1_STAT0_FINISHED;
	LOG (( "thmfc_floppy_find_sector: sector not found drive=%i track=%i sector=%i\n", image_index_in_device( img ), thmfc1->track, thmfc1->sector ));
	return 0;
}



/* complete command (by read, write, or timeout) */
static void thmfc_floppy_cmd_complete(void)
{
	LOG (( "%f thmfc_floppy_cmd_complete_cb: cmd=%i off=%i/%i/%i\n", 
	       attotime_to_double(timer_get_time()), thmfc1->op, thmfc1->data_idx, 
	       thmfc1->data_finish - 1, thmfc1->data_size - 1 ));

	if ( thmfc1->op == THMFC1_OP_WRITE_SECT ) 
	{
		/* TODO: detect ddam (?) */
		mess_image * img = thmfc_floppy_image();
		floppy_drive_write_sector_data( img, thmfc1->side, thmfc1->sector, thmfc1->data + 3, thmfc1->data_size - 3, 0 );
	}
	thmfc1->op = THMFC1_OP_RESET;
	thmfc1->stat0 |= THMFC1_STAT0_FINISHED;
	thmfc1->data_idx = 0;
	thmfc1->data_size = 0;
	timer_adjust( thmfc_floppy_cmd, attotime_never, 0, attotime_never );
}



static TIMER_CALLBACK( thmfc_floppy_cmd_complete_cb )
{
	thmfc_floppy_cmd_complete();
}



/* intelligent read: show just one field, skip header */
static UINT8 thmfc_floppy_read_byte ( void )
{
	UINT8 data = thmfc1->data[ thmfc1->data_idx ];
  
	VLOG(( "%f $%04x thmfc_floppy_read_byte: off=%i/%i/%i data=$%02X\n", 
	       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), 
	       thmfc1->data_idx, thmfc1->data_finish - 1, thmfc1->data_size - 1, 
	       data ));
  
	if ( thmfc1->data_idx >= thmfc1->data_size - 1 )
		thmfc_floppy_cmd_complete();
	else 
		thmfc1->data_idx++;

	if ( thmfc1->data_idx >= thmfc1->data_finish )
		thmfc1->stat0 |= THMFC1_STAT0_FINISHED;
	
	return data;
}



/* dumb read: show whole track with field headers and gaps  */
static UINT8 thmfc_floppy_raw_read_byte ( void )
{
	UINT8 data;
  
	/* rebuild track if needed */
	if ( ! thmfc1->data_raw_size )
	{
		if ( thmfc_floppy_is_qdd() )
			/* QDD: track = whole disk */
			thmfc1->data_raw_size = thom_qdd_make_disk ( thmfc_floppy_image(), thmfc1->data );
		else 
		{
			thmfc1->data_raw_idx = 0;
			thmfc1->data_raw_size = thom_floppy_make_track( thmfc_floppy_image(), thmfc1->data, 
									thmfc1->sector_size, thmfc1->side );
		}
		assert( thmfc1->data_raw_size < sizeof( thmfc1->data ) );
	}
  
	if ( thmfc1->data_raw_idx >= thmfc1->data_raw_size ) 
		data = 0;
	else 
		data = thmfc1->data[ thmfc1->data_raw_idx ];

	VLOG(( "%f $%04x thmfc_floppy_raw_read_byte: off=%i/%i data=$%02X\n", 
	       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), 
	       thmfc1->data_raw_idx, thmfc1->data_raw_size, data ));
  
	thmfc1->data_raw_idx++;
  
	return data;
}  



/* QDD writing / formating */
static void thmfc_floppy_qdd_write_byte ( UINT8 data )
{
	int i;

	if ( thmfc1->formatting && 
	     ( thmfc1->data_idx || data==0xA5 || data==0x5A ) &&  
	     thmfc1->data_raw_idx < THOM_MAXBUF ) 
	{
    
		if ( ! thmfc1->data_raw_size ) 
		{
			thmfc1->data_raw_size = thom_qdd_make_disk ( thmfc_floppy_image(), thmfc1->data );
			assert( thmfc1->data_raw_size < sizeof( thmfc1->data ) );
		}
    
		/* accumulate bytes to form a field */
		thmfc1->data[ thmfc1->data_raw_idx ] = data;
		thmfc1->data_raw_idx++;
    
		if ( ! thmfc1->data_idx  ) 
		{ 
			/* start */
			thmfc1->data_crc = 0;
			thmfc1->data_idx = thmfc1->data_raw_idx;
		}
    
		VLOG (( "%f $%04x thmfc_floppy_qdd_write_byte: $%02X offs=%i-%i\n", 
			attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data,
			thmfc1->data_idx,thmfc1->data_raw_idx ));
    
		if ( thmfc1->data_raw_idx == thmfc1->data_idx + 3 &&
		     thmfc1->data[ thmfc1->data_idx - 1 ] == 0xA5 && 
		     thmfc1->data[ thmfc1->data_idx + 2 ] == thmfc1->data_crc ) 
		{
      
			/* got an id field => format */
			int sector = (int) thmfc1->data[ thmfc1->data_idx ] * 256 + (int) thmfc1->data[ thmfc1->data_idx + 1 ];
			UINT8 filler = 0xff;
      
			LOG(( "%f $%04x thmfc_floppy_qdd_write_byte: id field, sector=%i\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), sector ));
      
			floppy_drive_format_sector( thmfc_floppy_image(), 0, sector, 0, 0, sector, 128, filler );
			thmfc1->data_idx = 0;
		}
    
		else if ( thmfc1->data_raw_idx == thmfc1->data_idx + 129 &&
			  thmfc1->data[ thmfc1->data_idx -   1 ] == 0x5A && 
			  thmfc1->data[ thmfc1->data_idx + 128 ] == thmfc1->data_crc ) 
		{
      
			/* look backwards for previous id field */
			for ( i = thmfc1->data_idx - 4; i >= 0; i-- )
			{
				if ( thmfc1->data[ i ] == 0xA5 &&
				     ( ( thmfc1->data[ i ] + thmfc1->data[ i + 1 ] + 
					 thmfc1->data[ i + 2 ] ) & 0xff 
					     ) == thmfc1->data[ i + 3 ] )
					break;
			}
      
			if ( i >= 0 ) 
			{
				/* got an id & a data field => write */
				mess_image * img = thmfc_floppy_image();
				int sector = (int) thmfc1->data[ i + 1 ] * 256 + 
					(int) thmfc1->data[ i + 2 ];
	
				LOG(( "%f $%04x thmfc_floppy_qdd_write_byte: data field, sector=%i\n", 
				      attotime_to_double(timer_get_time()), activecpu_get_previouspc(), sector ));
				
				floppy_drive_write_sector_data( img, 0, sector, thmfc1->data + thmfc1->data_idx, 128, 0 );
			}
			
			thmfc1->data_idx = 0;
      
		}
		else 
			thmfc1->data_crc += data;
	}
	else
	{
		thmfc1->data_raw_idx++;
		VLOG (( "%f $%04x thmfc_floppy_qdd_write_byte: ignored $%02X\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data ));
	}
  
}



/* intelligent writing */
static void thmfc_floppy_write_byte ( UINT8 data )
{
	VLOG (( "%f $%04x thmfc_floppy_write_byte: off=%i/%i data=$%02X\n", 
		attotime_to_double(timer_get_time()), activecpu_get_previouspc(), 
		thmfc1->data_idx, thmfc1->data_size - 1, data ));
  
	thmfc1->data_raw_size = 0;
	thmfc1->data[ thmfc1->data_idx ] = data;
	if ( thmfc1->data_idx >= thmfc1->data_size - 1 )
		thmfc_floppy_cmd_complete();
	else 
		thmfc1->data_idx++;
}

/* intelligent formatting */
static void thmfc_floppy_format_byte ( UINT8 data )
{
	VLOG (( "%f $%04x thmfc_floppy_format_byte: $%02X\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data ));
  
	thmfc1->data_raw_size = 0;
  
	/* accumulate bytes to form an id field */
	if ( thmfc1->data_idx || data==0xA1 ) 
	{
		UINT8 header[] = { 0xa1, 0xa1, 0xa1, 0xfe };
		thmfc1->data[ thmfc1->data_idx ] = data;
		thmfc1->data_idx++;
		if ( thmfc1->data_idx > 11 ) 
		{
      
			if ( !memcmp ( thmfc1->data, header, sizeof( header ) ) ) 
			{
				/* got id field => format */
				mess_image * img = thmfc_floppy_image();
				UINT8 track  = thmfc1->data[4];
				UINT8 side   = thmfc1->data[5];
				UINT8 sector = thmfc1->data[6];
				UINT8 length = thmfc1->data[7]; /* actually, log length */
				UINT8 filler = 0xe5;            /* standard Thomson filler */
				floppy_drive_format_sector( img, side, sector, track, thmfc1->side, sector, length, filler );
			}
			
			thmfc1->data_idx = 0;
		}
		
	}
}



READ8_HANDLER ( thmfc_floppy_r )
{
	switch ( offset ) 
	{
		
	case 0: /* STAT0 */
		thmfc1->stat0 ^= THMFC1_STAT0_SYNCHRO | THMFC1_STAT0_BYTE_READY_POL;
		VLOG(( "%f $%04x thmfc_floppy_r: STAT0=$%02X\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), thmfc1->stat0 ));
		return thmfc1->stat0;
    
	case 1: /* STAT1 */
	{
		UINT8 data = 0;
		mess_image * img = thmfc_floppy_image();
		int flags = floppy_drive_get_flag_state( img, -1 );
		if ( thmfc_floppy_is_qdd() ) 
		{
			if ( ! image_exists(img) ) 
				data |= 0x40; /* disk present */
			if ( ! thmfc1->ipl ) 
				data |= 0x02;       /* disk start */
			data |= 0x08; /* connected */
		}
		else 
		{
			if ( thmfc1->ipl ) 
				data |= 0x40;
			if ( image_exists(img) ) 
				data |= 0x20; /* disk change (?) */
			if ( flags & FLOPPY_DRIVE_HEAD_AT_TRACK_0 ) 
				data |= 0x08;
			if ( flags & FLOPPY_DRIVE_READY ) 
				data |= 0x02;
		}
		if ( ! (flags & FLOPPY_DRIVE_MOTOR_ON ) ) 
			data |= 0x10;
		if ( flags & FLOPPY_DRIVE_DISK_WRITE_PROTECTED ) 
			data |= 0x04;
		VLOG(( "%f $%04x thmfc_floppy_r: STAT1=$%02X\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data ));
		return data;
	}
	
	case 3: /* RDATA */
		
		if ( thmfc1->op == THMFC1_OP_READ_SECT || thmfc1->op == THMFC1_OP_READ_ADDR ) 
			return thmfc_floppy_read_byte();
		else
			return thmfc_floppy_raw_read_byte();

	case 6:
		return 0;
    
	case 8: 
	{ 
		/* undocumented => emulate TO7 QDD controller ? */
		UINT8 data = thmfc1->ipl << 7;
		VLOG(( "%f $%04x thmfc_floppy_r: STAT8=$%02X\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data ));
		return data;
	}

	default:
		logerror ( "%f $%04x thmfc_floppy_r: invalid read offset %i\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), offset );
		return 0;
	}
}



WRITE8_HANDLER ( thmfc_floppy_w )
{
	switch ( offset ) {

	case 0: /* CMD0 */
	{
		DENSITY dens = (data & 0x20) ? DEN_FM_LO : DEN_MFM_LO;
		int wsync = (data >> 4) & 1;
		int qdd = thmfc_floppy_is_qdd();
		chrn_id id;
		thom_floppy_set_density( dens );
		thmfc1->formatting = (data >> 2) & 1;
		LOG (( "%f $%04x thmfc_floppy_w: CMD0=$%02X dens=%s wsync=%i dsync=%i fmt=%i op=%i\n",
		       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data,  
		       (dens == DEN_FM_LO) ? "MF" : "MFM",
		       wsync, (data >> 3) & 1,
		       thmfc1->formatting, data & 3 ));
      
		/* abort previous command, if any */
		thmfc1->op = THMFC1_OP_RESET;
		timer_adjust( thmfc_floppy_cmd, attotime_never, 0, attotime_never );
      
		switch ( data & 3 ) 
		{

		case THMFC1_OP_RESET:
			thmfc1->stat0 = THMFC1_STAT0_FINISHED;
			break;
	
		case THMFC1_OP_WRITE_SECT:
			if ( qdd ) 
				logerror( "thmfc_floppy_w: smart operation 1 not supported for QDD\n" );
			else if ( thmfc_floppy_find_sector( NULL ) ) 
			{
				thmfc1->data_idx = 0;
				thmfc1->data_size = thmfc1->sector_size + 3; /* A1 A1 FB <data> */
				thmfc1->data_finish = thmfc1->sector_size + 3;
				thmfc1->stat0 |= THMFC1_STAT0_BYTE_READY_OP;
				thmfc1->op = THMFC1_OP_WRITE_SECT;
				timer_adjust( thmfc_floppy_cmd, ATTOTIME_IN_MSEC( 10 ), 0, attotime_never );
			}
			break;
	
		case THMFC1_OP_READ_ADDR:
			if ( qdd ) 
				logerror( "thmfc_floppy_w: smart operation 2 not supported for QDD\n" );
			else if ( thmfc_floppy_find_sector( &id ) ) 
			{
				thmfc1->data_size = 
					thom_floppy_make_addr( id, thmfc1->data, thmfc1->sector_size );
				assert( thmfc1->data_size < sizeof( thmfc1->data ) );
				thmfc1->data_finish = 10;
				thmfc1->data_idx = 1;
				thmfc1->stat0 |= THMFC1_STAT0_BYTE_READY_OP;
				thmfc1->op = THMFC1_OP_READ_ADDR;
				timer_adjust( thmfc_floppy_cmd, ATTOTIME_IN_MSEC( 1 ), 0, attotime_never );
			}
			break;
	
		case THMFC1_OP_READ_SECT:
			if ( qdd ) 
				logerror( "thmfc_floppy_w: smart operation 3 not supported for QDD\n" );
			else if ( thmfc_floppy_find_sector( &id ) ) 
			{
				thmfc1->data_size = thom_floppy_make_sector
					( thmfc_floppy_image(), id, thmfc1->data, thmfc1->sector_size );
				assert( thmfc1->data_size < sizeof( thmfc1->data ) );
				thmfc1->data_finish = thmfc1->sector_size + 4;
				thmfc1->data_idx = 1;
				thmfc1->stat0 |= THMFC1_STAT0_BYTE_READY_OP;
				thmfc1->op = THMFC1_OP_READ_SECT;
				timer_adjust( thmfc_floppy_cmd, ATTOTIME_IN_MSEC( 10 ), 0, attotime_never );
			}
			break;
		}

		/* synchronize to word, if needed (QDD only) */
		if ( wsync && qdd ) {
			if ( ! thmfc1->data_raw_size )
				thmfc1->data_raw_size = thom_qdd_make_disk ( thmfc_floppy_image(), thmfc1->data );
			while ( thmfc1->data_raw_idx < thmfc1->data_raw_size &&
				thmfc1->data[ thmfc1->data_raw_idx ] != thmfc1->wsync )
			{
				thmfc1->data_raw_idx++;
			}
		}
	}

	break;
    
    
	case 1: /* CMD1 */
		thmfc1->data_raw_size = 0;
		thmfc1->sector_size = 128 << ( (data >> 5) & 3);
		thmfc1->side = (data >> 4) & 1;
		if ( thmfc1->sector_size > 256 ) 
		{
			logerror( "$%04x thmfc_floppy_w: sector size %i > 256 not handled\n",
				  activecpu_get_previouspc(), thmfc1->sector_size );
			thmfc1->sector_size = 256;
		}
    
		LOG (( "%f $%04x thmfc_floppy_w: CMD1=$%02X sect-size=%i comp=%i head=%i\n",
		       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data,
		       thmfc1->sector_size, (data >> 1) & 7, thmfc1->side ));
		break;


	case 2: /* CMD2 */
	{
		mess_image * img;
		int seek = 0, motor;
		thmfc1->drive = data & 2;

		if ( thmfc_floppy_is_qdd() ) 
		{
			motor = !(data & 0x40);
			/* no side select & no seek for QDD */
		}
		else 
		{
			if ( data & 0x10 ) 
				seek = (data & 0x20) ? 1 : -1;
			motor =  (data >> 2) & 1;
			thmfc1->drive |= 1 ^ ((data >> 6) & 1);
		}

		img = thmfc_floppy_image();
		thom_floppy_active( 0 );
      
		LOG (( "%f $%04x thmfc_floppy_w: CMD2=$%02X drv=%i step=%i motor=%i\n",
		       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data, 
		       thmfc1->drive, seek, motor ));
 
		if ( seek ) 
		{ 
			thmfc1->data_raw_size = 0;
			floppy_drive_seek( img, seek );
		}

		/* in real life, to keep the motor running, it is sufficient to
		   set motor to 1 every few seconds.
		   instead of counting, we assume the motor is always running...
		*/
		floppy_drive_set_motor_state( img, 1 /* motor */ );
	}
	break;


	case 3: /* WDATA */
		thmfc1->wsync = data;
		if ( thmfc_floppy_is_qdd() ) 
			thmfc_floppy_qdd_write_byte( data );
		else if ( thmfc1->op==THMFC1_OP_WRITE_SECT ) 
			thmfc_floppy_write_byte( data );
		else if ( thmfc1->formatting ) 
			thmfc_floppy_format_byte( data );
		else 
		{
			/* TODO: implement other forms of raw track writing */
			LOG (( "%f $%04x thmfc_floppy_w: ignored raw WDATA $%02X\n", 
			       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data ));
		}
		break;
    

	case 4: /* WCLK (unemulated) */
		/* clock configuration: FF for data, 0A for synchro */
		LOG (( "%f $%04x thmfc_floppy_w: WCLK=$%02X (%s)\n", 
		       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data,
		       (data == 0xff) ? "data" : (data == 0x0A) ? "synchro" : "?" ));
		break;

	case 5: /* WSECT */
		thmfc1->sector = data;
		LOG (( "%f $%04x thmfc_floppy_w: WSECT=%i\n", 
		       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data ));
		break;

	case 6: /* WTRCK */
		thmfc1->track = data;
		LOG (( "%f $%04x thmfc_floppy_w: WTRCK=%i (real=%i)\n", 
		       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data,
		       floppy_drive_get_current_track( thmfc_floppy_image() ) ));
		break;

	case 7: /* WCELL */
		/* precompensation (unemulated) */
		LOG (( "%f $%04x thmfc_floppy_w: WCELL=$%02X\n", 
		       attotime_to_double(timer_get_time()), activecpu_get_previouspc(), data ));
		break;
      
	default:
		logerror ( "%f $%04x thmfc_floppy_w: invalid write offset %i (data=$%02X)\n", 
			   attotime_to_double(timer_get_time()), activecpu_get_previouspc(), offset, data );
	}
}



void thmfc_floppy_reset( void )
{
	int i;
	LOG(( "thmfc_floppy_reset: THMFC1 controller\n" ));
  
	for ( i = 0; i < device_count( IO_FLOPPY ); i++ ) 
	{
		mess_image * img = image_from_devtype_and_index( IO_FLOPPY, i );
		floppy_drive_set_index_pulse_callback( img, thmfc_floppy_index_pulse_cb );
		floppy_drive_set_ready_state( img, FLOPPY_DRIVE_READY, 0 );
		floppy_drive_seek( img, - floppy_drive_get_current_track( img ) );
	}

	thom_floppy_set_density( DEN_MFM_LO );
	thmfc1->op = THMFC1_OP_RESET;
	thmfc1->track = 0;
	thmfc1->sector = 0;
	thmfc1->side = 0;
	thmfc1->drive = 0;
	thmfc1->sector_size = 256;
	thmfc1->formatting = 0;
	thmfc1->stat0 = 0;
	thmfc1->data_idx = 0;
	thmfc1->data_size = 0;
	thmfc1->data_raw_idx = 0;
	thmfc1->data_raw_size = 0;
	thmfc1->data_crc = 0;
	thmfc1->wsync = 0;
	timer_adjust( thmfc_floppy_cmd, attotime_never, 0, attotime_never );
}



void thmfc_floppy_init( void )
{
	LOG(( "thmfc_floppy_init: THMFC1 controller\n" ));

	thmfc1 = auto_malloc( sizeof( *thmfc1 ) );
	assert( thmfc1 );

	thmfc_floppy_cmd = timer_alloc( thmfc_floppy_cmd_complete_cb, NULL );

	state_save_register_global( thmfc1->op );
	state_save_register_global( thmfc1->sector );
	state_save_register_global( thmfc1->track );
	state_save_register_global( thmfc1->side );
	state_save_register_global( thmfc1->drive );
	state_save_register_global( thmfc1->sector_size );
	state_save_register_global( thmfc1->formatting );
	state_save_register_global( thmfc1->ipl );
	state_save_register_global( thmfc1->data_idx );
	state_save_register_global( thmfc1->data_size );
	state_save_register_global( thmfc1->data_finish );
	state_save_register_global( thmfc1->stat0 );
	state_save_register_global( thmfc1->data_raw_idx );
	state_save_register_global( thmfc1->data_raw_size );
	state_save_register_global( thmfc1->data_crc );
	state_save_register_global( thmfc1->wsync );
	state_save_register_global_array( thmfc1->data );
}



/*********************** Network ************************/

/* The network extension is built as an external floppy controller.
   It uses the same ROM and I/O space, and so, it is natural to have the
   toplevel network emulation here!
*/

/* NOTE: This is work in progress! 
   For the moment, only hand-checks works: the TO7 can take the line, then
   perform a DKBOOT request. We do not have the server emulated yet, so,
   no way to answer the request.
*/

static TIMER_CALLBACK( ans4 )
{
	LOG(( "%f ans4\n", attotime_to_double(timer_get_time()) ));
	mc6854_set_cts( 0 );
}

static TIMER_CALLBACK( ans3 )
{
	LOG(( "%f ans3\n", attotime_to_double(timer_get_time()) ));
	mc6854_set_cts( 1 );
	timer_set(  ATTOTIME_IN_USEC( 100 ), NULL, 0, ans4 );
}

static TIMER_CALLBACK( ans2 )
{
	LOG(( "%f ans2\n", attotime_to_double(timer_get_time()) ));
	mc6854_set_cts( 0 );
	timer_set(  ATTOTIME_IN_USEC( 100 ), NULL, 0, ans3 );
}

static TIMER_CALLBACK( ans )
{
	LOG(( "%f ans\n", attotime_to_double(timer_get_time()) ));
	mc6854_set_cts( 1 );
	timer_set(  ATTOTIME_IN_USEC( 100 ), NULL, 0, ans2 );
}
/* consigne DKBOOT

   MO5 BASIC
   $00 $00 $01 $00 $00 $00 $00 $00 $00 $00 $01 $00<$41 $00 $FF $20 
   $3D $4C $01 $60 $20 $3C $4F $01 $05 $20 $3F $9C $19 $25 $03 $11
   $93 $15 $10 $25 $32 $8A $7E $FF $E1 $FD $E9 $41>$00 $00 $00 $00 
   $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00

   TO7/70 BASIC
   $00 $00 $01 $00 $00 $00 $00 $00 $00 $00 $02 $00<$20 $42 $41 $53 
   $49 $43 $20 $4D $49 $43 $52 $4F $53 $4F $46 $54 $20 $31 $2E $30
   $04 $00 $00 $00 $00 $00 $60 $FF $37 $9B $37 $9C>$00 $00 $00 $00 
   $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00

   TO7 BASIC
   $00 $00 $01 $00 $00 $00 $00 $00 $00 $00 $00 $00<$20 $42 $41 $53 
   $49 $43 $20 $4D $49 $43 $52 $4F $53 $4F $46 $54 $20 $31 $2E $30
   $04 $00 $00 $00 $00 $00 $60 $FF $37 $9B $37 $9C>$00 $00 $00 $00 
   $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00

   TO7 LOGO
   $00 $00 $01 $00 $00 $00 $00 $00 $00 $00 $00 $00<$00 $00 $00 $00 
   $00 $20 $4C $4F $47 $4F $04 $00 $00 $00 $00 $00 $00 $00 $00 $00
   $00 $00 $00 $00 $00 $00 $AA $FF $01 $16 $00 $C8>$00 $00 $00 $00 
   $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00 $00


*/

static void to7_network_got_frame( UINT8* data, int length )
{
	int i;
	LOG(( "%f to7_network_got_frame:", attotime_to_double(timer_get_time()) ));
	for ( i = 0; i < length; i++ )
		LOG(( " $%02X", data[i] ));
	LOG(( "\n" ));

	if ( data[1] == 0xff ) 
	{
		LOG(( "to7_network_got_frame: %i phones %i\n", data[2], data[0] ));
		timer_set(  ATTOTIME_IN_USEC( 100 ), NULL, 0, ans  );
		mc6854_set_cts( 0 );
	}
	else if ( ! data[1] ) 
	{
		char name[33];
		int i;
		memcpy( name, data + 12, 32 );    
		name[32] = 0;
		for (i=0;i<32;i++) 
		{
			if ( name[i]<32 || name[i]>=127 ) 
				name[i]=' ';
		}
		LOG(( "to7_network_got_frame: DKBOOT system=%s appli=\"%s\"\n",
		      (data[10] == 0) ? "TO7" : (data[10] == 1) ? "MO5" :
		      (data[10] == 2) ? "TO7/70" : "?", name ));
	}
  
}



static const mc6854_interface network_iface = { NULL, to7_network_got_frame, NULL, NULL };



static void to7_network_init( void )
{
	LOG(( "to7_network_init: NR 07-005 network extension\n" ));
	logerror( "to7_network_init: network not handled!\n" );
	mc6854_config( &network_iface  );
}



static void to7_network_reset( void )
{
	LOG(( "to7_network_reset: NR 07-005 network extension\n" ));
	mc6854_reset();
	mc6854_set_cts( 1 );
}



static READ8_HANDLER ( to7_network_r )
{
	if ( offset >= 0 && offset < 4 ) 
		return mc6854_r( offset );

	if ( offset == 8 ) 
	{
		/* network ID of the computer */
		UINT8 id = readinputport( THOM_INPUT_FCONFIG ) >> 3;
		VLOG(( "%f $%04x to7_network_r: read id $%02X\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), id ));
		return id;
	}

	logerror( "%f $%04x to7_network_r: invalid read offset %i\n", attotime_to_double(timer_get_time()), activecpu_get_previouspc(), offset );
	return 0;
}



static WRITE8_HANDLER ( to7_network_w )
{
	if ( offset >= 0 && offset < 4 )
		mc6854_w( offset, data );
	else
	{
		logerror( "%f $%04x to7_network_w: invalid write offset %i (data=$%02X)\n",
			  attotime_to_double(timer_get_time()), activecpu_get_previouspc(), offset, data );
	}
}



/*********************** TO7 dispatch ************************/

/* The TO7, TO7/70, MO5 and MO6 can use a variety of floppy controllers.
   We use a PORT_CONFSETTING to chose the current controller, and dispatch
   here.

   NOTE: you need to reset the computer after changing the floppy controller!

   The CD 90-351 controller seems similar to the THMFC1 gate-array 
   => we reuse the THMFC code!
*/



UINT8 to7_controller_type;
UINT8 to7_floppy_bank;


void to7_floppy_init ( void* base )
{
	memory_configure_bank( THOM_FLOP_BANK, 0, TO7_NB_FLOP_BANK, base, 0x800 );
	state_save_register_global( to7_controller_type );
	state_save_register_global( to7_floppy_bank );
	to7_5p14sd_init();
	to7_5p14_init();
	to7_qdd_init();
	thmfc_floppy_init();
	to7_network_init();
}



void to7_floppy_reset ( void )
{
	to7_controller_type = (readinputport( THOM_INPUT_FCONFIG ) ) & 7;

	switch ( to7_controller_type ) 
	{

	case 1:
		to7_floppy_bank = 1;
		to7_5p14sd_reset();
		break;

	case 2:
		to7_floppy_bank = 2;
		to7_5p14_reset(); 
		break;

	case 3:
		to7_floppy_bank = 3;
		thmfc_floppy_reset();
		break;

	case 4:  
		to7_floppy_bank = 7;
		to7_qdd_reset(); 
		break;

	case 5:  
		to7_floppy_bank = 8;
		to7_network_reset(); 
		break;

	default:
		to7_floppy_bank = 0;
		break;
	}

	memory_set_bank( THOM_FLOP_BANK, to7_floppy_bank );
}



READ8_HANDLER ( to7_floppy_r )
{
	switch ( to7_controller_type ) 
	{

	case 1:  
		return to7_5p14sd_r( offset );

	case 2:  
		return to7_5p14_r( offset );
    
	case 3:  
		return thmfc_floppy_r( offset );
    
	case 4:  
		return to7_qdd_r( offset );

	case 5:  
		return to7_network_r( offset );
	}

	return 0;
}



WRITE8_HANDLER ( to7_floppy_w )
{
	switch ( to7_controller_type ) 
	{
    
	case 1:
		to7_5p14sd_w( offset, data );
		return;

	case 2:
		to7_5p14_w( offset, data ); 
		break;
    
	case 3:
		if ( offset == 8 ) 
		{
			to7_floppy_bank = 3 + (data & 3);
			memory_set_bank( THOM_FLOP_BANK,  to7_floppy_bank );
			VLOG (( "to7_floppy_w: set CD 90-351 ROM bank to %i\n", data & 3 ));
		}
		else 
			thmfc_floppy_w( offset, data ); 
		break;

	case 4:
		to7_qdd_w( offset, data ); 
		break;

	case 5:  
		to7_network_w( offset, data ); 
		break;
	}
}



/*********************** TO9 ************************/

/* the internal controller is WD2793-based (so, similar to to7_5p14) 
   we also emulate external controllers
*/



void to9_floppy_init( void* int_base, void* ext_base )
{
	to7_floppy_init( ext_base );
	memory_configure_bank( THOM_FLOP_BANK, TO7_NB_FLOP_BANK, 1, int_base, 0x800 );
}



void to9_floppy_reset( void )
{
	to7_floppy_reset();
	if ( THOM_FLOPPY_EXT ) 
	{
		LOG(( "to9_floppy_reset: external controller\n" ));
	}
	else
	{
		LOG(( "to9_floppy_reset: internal controller\n" ));
		to7_5p14_reset();
		memory_set_bank( THOM_FLOP_BANK, TO7_NB_FLOP_BANK );
	}
}



READ8_HANDLER ( to9_floppy_r )
{
	if ( THOM_FLOPPY_EXT ) 
		return to7_floppy_r( offset );
	else 
		return  to7_5p14_r( offset );
}

WRITE8_HANDLER ( to9_floppy_w )
{
	if ( THOM_FLOPPY_EXT ) 
		to7_floppy_w( offset, data );
	else 
		to7_5p14_w( offset, data );
}
