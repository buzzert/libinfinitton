/*
 * infdevice.h
 *
 * Created 2019-02-13 by James Magahern <james@magahern.com>
 */

#pragma once

#include "pixmap.h"

struct infdevice_t_;
typedef struct infdevice_t_ infdevice_t;

// If the device exists, returns a handle to it. Otherwise, returns NULL
extern infdevice_t* infdevice_open ();

// Close and cleanup the device
extern void infdevice_close (infdevice_t *device);

// Sets a pixmap (icon) to a particular key_id, where key_id is a number between 0 and 15.
extern void infdevice_set_pixmap_for_key_id (infdevice_t *device, 
                                             const unsigned key_id, 
                                             infpixmap_t *pixmap);


