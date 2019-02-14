/*
 * infctl
 *
 * Created on 2019-02-13 by James Magahern <james@magahern.com>
 */

#include <infinitton.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

    infdevice_t *device = infdevice_open ();
    if (!device) {
        fprintf(stderr, "Could not open device\n");
        return 1;
    }

    cairo_surface_t *surface = infpixmap_create_surface ();
    cairo_t *cr = cairo_create (surface);

    unsigned color_idx = 0;
    double color[3] = { 0, 0, 0 };

    int num = 1;
    for (;;) {
        cairo_set_source_rgb (cr, color[0], color[1], color[2]);
        cairo_paint (cr);

        infpixmap_t *pixmap = infpixmap_create (surface);
        infdevice_set_pixmap_for_key_id (device, 1 + num, pixmap);
        infpixmap_free (pixmap);

        num = (num + 1) % 16;
        
        color[color_idx] += 0.2;
        if (color[color_idx] >= 1.0) {
            color_idx = (color_idx + 1) % 3;
            color[color_idx] = 0;
        }

        usleep(5000);
    }

    /*

    infpixmap_t *pixmap = infpixmap_open_file (bmp_path);
    if (!pixmap) {
        fprintf(stderr, "Unable to open BMP file\n");
        return 1;
    }

    infdevice_set_pixmap_for_key_id (device, key_id, pixmap);

    infpixmap_free (pixmap);
    */

    infdevice_close (device);

    return 0;
}

