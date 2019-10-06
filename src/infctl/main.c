/*
 * infctl
 *
 * Created on 2019-02-13 by James Magahern <james@magahern.com>
 */

#include <infinitton.h>

#include <pango/pangocairo.h>

#include <math.h>
#include <stdbool.h>
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

        infkey_t key = infkey_num_to_key (num);

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

    infkey_t key_id = infkey_num_to_key (keynum);
    infdevice_set_pixmap_for_key_id (device, key_id, pixmap);
    infpixmap_free (pixmap);
}

static void test_reading (infdevice_t *device, char **argv)
{
    cairo_surface_t *surface = infpixmap_create_surface ();
    cairo_t *cr = cairo_create (surface);
    infpixmap_t *pixmap = infpixmap_create (surface);

    PangoLayout *layout = pango_cairo_create_layout (cr);
    PangoFontDescription *font = pango_font_description_from_string ("Sans 24");
    if (font == NULL) {
        fprintf (stderr, "Could not find font\n");
        return;
    }

    pango_layout_set_font_description (layout, font);
    pango_font_description_free (font);

    infkey_t pressed_key = INF_KEY_CLEARED;
    for (;;) {
        for (unsigned int keynum = 0; keynum < INF_NUM_KEYS; keynum++) {
            infkey_t key = infkey_num_to_key (keynum);
            bool highlighted = (key & pressed_key);

            // Rotate 90 deg
            cairo_save (cr);
            cairo_translate (cr, ICON_WIDTH / 2, ICON_HEIGHT / 2);
            cairo_rotate (cr, M_PI_2);
            cairo_translate (cr, -ICON_WIDTH / 2, -ICON_HEIGHT / 2);

            // Background color
            cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
            if (highlighted) {
                cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
            }

            cairo_paint (cr);

            // Draw number label
            cairo_set_source_rgb (cr, 0.0, 1.0, 1.0);
            if (highlighted) {
                cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
            }

            enum { NUM_STR_LEN = 3 };
            char num_str[NUM_STR_LEN];
            snprintf (num_str, NUM_STR_LEN, "%d", keynum);

            pango_layout_set_text (layout, num_str, -1);

            int width; int height;
            pango_layout_get_pixel_size (layout, &width, &height);

            cairo_move_to (cr, (ICON_WIDTH - width) / 2.0, (ICON_HEIGHT - height) / 2.0);
            pango_cairo_show_layout (cr, layout);

            infpixmap_update_with_surface (pixmap, surface);
            infdevice_set_pixmap_for_key_id (device, key, pixmap);

            cairo_restore (cr);
        }

        pressed_key = infdevice_read_key (device);

        int pressed_keynum = infkey_to_key_num (pressed_key);
        if (pressed_keynum > 0) {
            printf("%d\n", pressed_keynum - 1);
        }
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

