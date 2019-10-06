/*
 * infctl
 *
 * Created on 2019-02-13 by James Magahern <james@magahern.com>
 */

#include <infinitton.h>

#include <pango/pangocairo.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void test_dynamic_pixmap (infdevice_t *device, char **args);
static void test_pixmap_bmp (infdevice_t *device, char **argv);
static void test_reading (infdevice_t *device, char **argv);

typedef struct {
    const char *name;
    void (*function)(infdevice_t*, char**);
} command_t;

static command_t __commands[] = {
    { "pixmap", test_dynamic_pixmap },
    { "bmp",    test_pixmap_bmp },
    { "read",   test_reading },
};

static void print_usage (const char *progname)
{
    fprintf (stderr, "Usage: %s [command] [arguments...]\n", progname);
    fprintf (stderr, "Commands: \n");
    fprintf (stderr, "\tbmp [key_id] [bmp_file]: Load BMP file\n");
    fprintf (stderr, "\t\tBMP file must be 72x72, 24-bits (R8 G8 B8), no colorspace info\n");
    fprintf (stderr, "\tpixmap: Test dynamically generated pixmaps\n");
    fprintf (stderr, "\tread: Test reading input\n");
}

static void test_dynamic_pixmap (infdevice_t *device, char **args)
{
    cairo_surface_t *surface = infpixmap_create_surface ();
    cairo_t *cr = cairo_create (surface);
    infpixmap_t *pixmap = infpixmap_create (surface);

    unsigned color_idx = 0;
    double color[3] = { 0, 0, 0 };

    int num = 1;
    for (;;) {
        cairo_set_source_rgb (cr, color[0], color[1], color[2]);
        cairo_paint (cr);

        inf_key_t key = inf_key_num_to_key (num);

        if (key != INF_KEY_CLEARED) {
            infpixmap_update_with_surface (pixmap, surface);
            infdevice_set_pixmap_for_key_id (device, key, pixmap);
        }

        num = (num + 1) % 16;
        
        color[color_idx] += 0.2;
        if (color[color_idx] >= 1.0) {
            color_idx = (color_idx + 1) % 3;
            color[color_idx] = 0;
        }

        usleep(5000);
    }

    infpixmap_free (pixmap);
    cairo_surface_destroy (surface);
}

static void test_pixmap_bmp (infdevice_t *device, char **argv)
{
    const int keynum = strtod (argv[1], NULL);
    const char *bmp_path = argv[2];

    infpixmap_t *pixmap = infpixmap_open_file (bmp_path);
    if (!pixmap) {
        fprintf(stderr, "Unable to open BMP file\n");
        return;
    }

    inf_key_t key_id = inf_key_num_to_key (keynum);
    infdevice_set_pixmap_for_key_id (device, key_id, pixmap);
    infpixmap_free (pixmap);
}

static void test_reading (infdevice_t *device, char **argv)
{
    cairo_surface_t *surface = infpixmap_create_surface ();
    cairo_t *cr = cairo_create (surface);
    infpixmap_t *pixmap = infpixmap_create (surface);

    inf_key_t pressed_key = INF_KEY_CLEARED;
    for (;;) {
        for (unsigned int keynum = 0; keynum < INF_NUM_KEYS; keynum++) {
            inf_key_t key = inf_key_num_to_key (keynum);

            // Refresh screen
            cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
            if (key & pressed_key) {
                cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
            }

            cairo_paint (cr);
            infpixmap_update_with_surface (pixmap, surface);

            infdevice_set_pixmap_for_key_id (device, key, pixmap);
        }

        pressed_key = infdevice_read_key (device);
    }

    infpixmap_free (pixmap);
    cairo_surface_destroy (surface);
}

int main (int argc, char **argv)
{
    if (argc < 2) {
        print_usage (argv[0]);
        return 1;
    }

    infdevice_t *device = infdevice_open ();
    if (!device) {
        fprintf(stderr, "Could not open device\n");
        return 1;
    }

    unsigned int command_idx = 0;
    size_t num_commands = sizeof (__commands) / sizeof (command_t);
    for (; command_idx < num_commands; command_idx++) {
        const command_t command = __commands[command_idx];
        if (strncmp (argv[1], command.name, strlen(command.name)) == 0) {
            command.function (device, argv + 1);
            break;
        }
    }

    if (command_idx == num_commands) {
        fprintf (stderr, "Command not found\n");
        print_usage (argv[0]);
    }

    infdevice_close (device);

    return 0;
}

