/****************************************************************************

	image.h

	Code for handling devices/software images

****************************************************************************/

#ifndef IMAGE_H
#define IMAGE_H

#include <stdlib.h>
#include "fileio.h"
#include "utils.h"
#include "opresolv.h"
#include "device.h"
#include "osdmess.h"

typedef enum
{
	IMAGE_ERROR_SUCCESS,
	IMAGE_ERROR_INTERNAL,
	IMAGE_ERROR_UNSUPPORTED,
	IMAGE_ERROR_OUTOFMEMORY,
	IMAGE_ERROR_FILENOTFOUND,
	IMAGE_ERROR_INVALIDIMAGE,
	IMAGE_ERROR_ALREADYOPEN,
	IMAGE_ERROR_UNSPECIFIED
} image_error_t;

struct _images_private;
typedef struct _images_private images_private;



/****************************************************************************
  A const device_config pointer represents one device entry; this could be an instance
  of a floppy drive or a printer.  The defining property is that either at
  startup or at runtime, it can become associated with a file on the hosting
  machine (for the examples above, a disk image or printer output file
  repsectively).  In fact, const device_config might be better renamed mess_device.

  MESS drivers declare what device types each system had and how many.  For
  each of these, a const device_config is allocated.  Devices can have init functions
  that allow the devices to set up any relevant internal data structures.
  It is recommended that devices use the tag allocation system described
  below.

  There are also various other accessors for other information about a
  device, from whether it is mounted or not, or information stored in a CRC
  file.
****************************************************************************/

/* not to be called by anything other than core */
int image_init(running_machine *machine);



/****************************************************************************
  Device loading and unloading functions

  The UI can call image_load and image_unload to associate and disassociate
  with disk images on disk.  In fact, devices that unmount on their own (like
  Mac floppy drives) may call this from within a driver.
****************************************************************************/

/* can be called by front ends */
int image_load(const device_config *img, const char *name);
int image_create(const device_config *img, const char *name, int create_format, option_resolution *create_args);
void image_unload(const device_config *img);

/* used to retrieve error information during image loading */
const char *image_error(const device_config *img);

/* used for driver init and machine init */
void image_unload_all(int ispreload);

/* used to set the error that occured during image loading */
void image_seterror(const device_config *img, image_error_t err, const char *message);



/****************************************************************************
  Tag management functions.

  When devices have private data structures that need to be associated with a
  device, it is recommended that image_alloctag() be called in the device
  init function.  If the allocation succeeds, then a pointer will be returned
  to a block of memory of the specified size that will last for the lifetime
  of the emulation.  This pointer can be retrieved with image_lookuptag().

  Note that since image_lookuptag() is used to index preallocated blocks of
  memory, image_lookuptag() cannot fail legally.  In fact, an assert will be
  raised if this happens
****************************************************************************/

void *image_alloctag(const device_config *img, const char *tag, size_t size);
void *image_lookuptag(const device_config *img, const char *tag);



/****************************************************************************
  Accessor functions

  These provide information about the device; and about the mounted image
****************************************************************************/

const struct IODevice *image_device(const device_config *image);
int image_exists(const device_config *image);
int image_slotexists(const device_config *image);

core_file *image_core_file(const device_config *image);
const char *image_typename_id(const device_config *image);
const char *image_filename(const device_config *image);
const char *image_basename(const device_config *image);
const char *image_basename_noext(const device_config *image);
const char *image_filetype(const device_config *image);
const char *image_filedir(const device_config *image);
const char *image_working_directory(const device_config *image);
void image_set_working_directory(const device_config *image, const char *working_directory);
UINT64 image_length(const device_config *image);
const char *image_hash(const device_config *image);
UINT32 image_crc(const device_config *image);

int image_is_writable(const device_config *image);
int image_has_been_created(const device_config *image);
void image_make_readonly(const device_config *image);

UINT32 image_fread(const device_config *image, void *buffer, UINT32 length);
UINT32 image_fwrite(const device_config *image, const void *buffer, UINT32 length);
int image_fseek(const device_config *image, INT64 offset, int whence);
UINT64 image_ftell(const device_config *image);
int image_fgetc(const device_config *image);
int image_feof(const device_config *image);

void *image_ptr(const device_config *image);



/****************************************************************************
  Memory allocators

  These allow memory to be allocated for the lifetime of a mounted image.
  If these (and the above accessors) are used well enough, they should be
  able to eliminate the need for a unload function.
****************************************************************************/

void *image_malloc(const device_config *img, size_t size) ATTR_MALLOC;
char *image_strdup(const device_config *img, const char *src) ATTR_MALLOC;
void *image_realloc(const device_config *img, void *ptr, size_t size);
void image_freeptr(const device_config *img, void *ptr);



/****************************************************************************
  CRC Accessor functions

  When an image is mounted; these functions provide access to the information
  pertaining to that image in the CRC database
****************************************************************************/

const char *image_longname(const device_config *img);
const char *image_manufacturer(const device_config *img);
const char *image_year(const device_config *img);
const char *image_playable(const device_config *img);
const char *image_extrainfo(const device_config *img);



/****************************************************************************
  Battery functions

  These functions provide transparent access to battery-backed RAM on an
  image; typically for cartridges.
****************************************************************************/

void image_battery_load(const device_config *img, void *buffer, int length);
void image_battery_save(const device_config *img, const void *buffer, int length);



/****************************************************************************
  Indexing functions

  These provide various ways of indexing images
****************************************************************************/

int image_absolute_index(const device_config *image);
const device_config *image_from_absolute_index(int absolute_index);



/****************************************************************************
  Deprecated functions

  The usage of these functions is to be phased out.  The first group because
  they reflect the outdated fixed relationship between devices and their
  type/id.
****************************************************************************/

int image_index_in_device(const device_config *img);
const device_config *image_from_device(const struct IODevice *dev);
const device_config *image_from_devtag_and_index(const char *devtag, int id);

/* deprecated; as there can be multiple devices of a certain type */
iodevice_t image_devtype(const device_config *img);
const device_config *image_from_devtype_and_index(iodevice_t type, int id);




/****************************************************************************
  Macros for declaring device callbacks
****************************************************************************/

#define	DEVICE_INIT(name)	int device_init_##name(const device_config *image)
#define DEVICE_EXIT(name)	void device_exit_##name(const device_config *image)
#define DEVICE_LOAD(name)	int device_load_##name(const device_config *image)
#define DEVICE_CREATE(name)	int device_create_##name(const device_config *image, int create_format, option_resolution *create_args)
#define DEVICE_UNLOAD(name)	void device_unload_##name(const device_config *image)

#endif /* IMAGE_H */
