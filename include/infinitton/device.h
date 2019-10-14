/*
 * infdevice.h
 *
 * Created 2019-02-13 by James Magahern <james@magahern.com>
 */

#pragma once

#include "pixmap.h"
#include "keys.h"

struct infdevice_t_;
typedef struct infdevice_t_ infdevice_t;

// If the device exists, returns a handle to it. Otherwise, returns NULL
extern infdevice_t* infdevice_open ();

// Close and cleanup the device
extern void infdevice_close (infdevice_t *device);

// Sets a pixmap (icon) to a particular key_id, see "keys.h" for what to pass as `key_id`.
extern void infdevice_set_pixmap_for_key_id (infdevice_t *device, 
                                             infkey_t    key_id, 
                                             infpixmap_t *pixmap);

// Returns a bitfield (defined in keys.h as infkey_t) representing which keys are currently
// being held down. A result of zero (INF_KEY_CLEARED) is sent for when all keys are released.
// Blocks the calling thread until a response is read from the device (until a key is pressed).
extern infkey_t infdevice_read_key (infdevice_t *device);

