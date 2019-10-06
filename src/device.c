/*
 * infdevice.c
 *
 * Created 2019-02-13 by James Magahern <james@magahern.com>
 */

#include <infinitton/infinitton.h>
#include <infinitton/util.h>

#include <hidapi/hidapi.h>

// TODO: platform deps
#include <linux/input.h>
#include <linux/input-event-codes.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define VENDOR_ID   0xFFFF
#define PRODUCT_ID  0x1F40

typedef struct __attribute__((__packed__)) {
    int32_t offset;
    int32_t length; // 0x1f40

    int32_t ctrl1;  // 0x55aaaa55
    int32_t ctrl2;  // 0x44332211
} binary_data_partial_t;

typedef struct __attribute__((__packed__)) {
    int8_t  a; // 0x12
    int16_t b; // 0x1
    char    c;
    
    int32_t screen_num;

    int32_t d; // 0x0
    int32_t e; // 0x0
    int32_t number; // always 15606

    char    padding[13];
} feature_packet_t;

typedef struct __attribute__((__packed__)) {
    int8_t  descriptor; // always seems to be 0x01.
    int16_t key_state;  // this is 0x0000 when a key goes up.  
} inf_input_t;

struct infdevice_t_ {
    hid_device *hid_device;
};

static void infdevice_write (infdevice_t *device, unsigned char *data, size_t len)
{
    if (util_debugging_enabled ()) {
        fwrite (data, len, 1, stdout);
    } else {
        hid_write (device->hid_device, data, len);
    }
}

static void infdevice_feature (infdevice_t *device, unsigned char *data, size_t len)
{
    if (util_debugging_enabled ()) {
        fwrite (data, len, 1, stdout);
    } else {
        hid_send_feature_report (device->hid_device, data, len);
    }
}

static void transfer_pixmap(infdevice_t *device, infpixmap_t *pixmap)
{
    size_t size = 0;
    unsigned char *pixmap_data = infpixmap_get_data (pixmap, &size);
    
    // Malloc payload and copy data after the header
    const int8_t report_id = 0x02;
    const size_t payload_size = 8000; // sent in two chunks
    const size_t header_size = sizeof (binary_data_partial_t) + 1;
    const size_t total_chunk_size = payload_size + header_size;
    
    size_t offset = 0;
    unsigned char *payload = (unsigned char *)calloc (total_chunk_size, 1);
    
    // Write report_id (0x02)
    memcpy (payload, &report_id, 1);
    offset += 1;

    // Write first header
    binary_data_partial_t header = {
        .offset = 0,
        .length = payload_size,
        .ctrl1  = 0x55aaaa55,
        .ctrl2  = 0x44332211
    };
    memcpy (payload + offset, &header, sizeof (header));
    offset += sizeof (header);

    // Write first half of image data
    memcpy (payload + offset, pixmap_data, total_chunk_size - offset);

    // TRANSMIT
    infdevice_write (device, payload, total_chunk_size);
    
    // Second payload
    memset (payload, 0, total_chunk_size);

    // report_id
    memcpy (payload, &report_id, 1);
    offset = 1;
    
    // Second header
    header.offset = payload_size;
    header.length = sizeof (header) + size - payload_size;
    memcpy (payload + offset, &header, sizeof (header));
    offset += sizeof (header);

    // Second half of image data
    memcpy (payload + offset, pixmap_data + payload_size, payload_size - offset);

    // TRANSMIT
    infdevice_write (device, payload, total_chunk_size);

    // Cleanup
    free(payload);
}

void send_feature (infdevice_t *device, int key_id, infpixmap_t *pixmap)
{
    size_t size = 0;
    infpixmap_get_data (pixmap, &size);

    feature_packet_t payload = {
        .a = 0x12, // 0x12
        .b = 0x01, // 0x1
        
        .screen_num = key_id,

        .d = 0x0,      // 0x0
        .e = 0x0,      // 0x0
        .number = size
    };

    infdevice_feature (device, (unsigned char *)&payload, sizeof (feature_packet_t));
}

infdevice_t* infdevice_open ()
{
    hid_device *hid_device = NULL;
    if (!util_debugging_enabled ()) {
        hid_device = hid_open (VENDOR_ID, PRODUCT_ID, NULL);
        if (hid_device == NULL) {
            fprintf (stderr, "Unable to open device: %s\n", (const char *)hid_error (hid_device));
            fprintf (stderr, "Check permissions?\n");
        }
    }

    struct infdevice_t_ *device = (struct infdevice_t_ *) malloc (sizeof (struct infdevice_t_));
    device->hid_device = hid_device;

    return device;
}

void infdevice_close (infdevice_t *device) 
{
    if (device->hid_device) {
        hid_close (device->hid_device);
    }

    free (device);
}

void infdevice_set_pixmap_for_key_id (infdevice_t *device, 
                                      infkey_t    key_id, 
                                      infpixmap_t *pixmap)
{
   transfer_pixmap (device, pixmap);

   // SUCKS that this appears to be necessary. Without this, all kinds of corruption
   // happens on the display.
   usleep (1500);

   unsigned int keynum = infkey_to_key_num (key_id);
   send_feature (device, keynum, pixmap);
}

infkey_t infdevice_read_key (infdevice_t *device)
{
    inf_input_t input_event;
    hid_read (device->hid_device, (unsigned char *)&input_event, sizeof (inf_input_t));

    return input_event.key_state;
}
