/*
 * infdevice.c
 *
 * Created 2019-02-13 by James Magahern <james@magahern.com>
 */

#include "../include/device.h"

#include <hidapi/hidapi.h>
#include <stdlib.h>
#include <stdio.h>

#define VENDOR_ID   0xFFFF
#define PRODUCT_ID  0x1F40

struct infdevice_t_ {
    hid_device *hid_device;
};

infdevice_t* infdevice_open ()
{
    hid_device *hid_device = hid_open (0xffff, 0x1f40, NULL);
    if (hid_device == NULL) {
        fprintf (stderr, "Unable to open device: %s\n", (const char *)hid_error (hid_device));
        fprintf (stderr, "Check permissions?\n");
        return NULL;
    }

    struct infdevice_t_ *device = (struct infdevice_t_ *) malloc (sizeof (struct infdevice_t_));
    device->hid_device = hid_device;

    return device;
}

void infdevice_close (infdevice_t *device) 
{
    hid_close (device->hid_device);
    free (device);
}

void infdevice_set_pixmap_for_key_id (infdevice_t *device, 
                                      const unsigned key_id, 
                                      infpixmap_t *pixmap)
{
    // TODO
}


