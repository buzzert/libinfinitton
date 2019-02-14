/*
 * infctl
 *
 * Created on 2019-02-13 by James Magahern <james@magahern.com>
 */

#include <infinitton.h>

#include <stdio.h>
#include <stdlib.h>

static void print_usage (const char *progname)
{
    fprintf(stderr, "Usage: %s [key_id] [bmp_file]\n", progname);
    fprintf(stderr, "BMP file must be 72x72, 24-bits (R8 G8 B8), no colorspace info\n");
}

int main (int argc, char **argv)
{
    if (argc < 3) {
        print_usage (argv[0]);
        return 1;
    }

    const int key_id = strtod (argv[1], NULL);
    const char *bmp_path = argv[2];

    infpixmap_t *pixmap = infpixmap_open_file (bmp_path);
    if (!pixmap) {
        fprintf(stderr, "Unable to open BMP file\n");
        return 1;
    }

    infdevice_t *device = infdevice_open ();
    infdevice_set_pixmap_for_key_id (device, key_id, pixmap);

    infdevice_close (device);
    infpixmap_free (pixmap);

    return 0;
}

