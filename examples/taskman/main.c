#include <infinitton/infinitton.h>

#include <assert.h>
#include <cairo/cairo.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/param.h> // max, min

#define TITLE_BUFSIZE 512
#define ICON_BUFSIZE 512 * 512

Display *__display;
Window   __root_window;
Window   __active_window;

typedef struct {
    Window           window;
    char            *title;
    cairo_surface_t *icon_surface;
    unsigned long   *icon_surface_data;
} application_t;

static application_t __apps_for_keys[INF_NUM_KEYS] = { 0 };
static unsigned int __num_running_apps = 0;
static bool __running = true;

// Atoms
static Atom __a_active_window;
static Atom __a_client_list;
static Atom __a_wm_name;
static Atom __a_wm_icon;

static int
horiz_key_order (int i)
{
    // Converts from a ordinal number to horizontal ordering of keys on the keypad
    return (i / 3) + ((i % 3) * 5);
}

static int
from_horiz_key_order (int i)
{
    return (3 * (i % 5)) + ((i / 5) % 3);
}

static void
raise_window_id (Window window)
{
    XEvent event = { 0 };
    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = __a_active_window;
    event.xclient.window = window;
    event.xclient.format = 32;

    XSendEvent (__display, __root_window, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);

    // xxx: X is not multithreaded, so this should be coordinated with the main thread instead.
    XFlush(__display);

    XMapRaised (__display, window);
}

static unsigned char *
get_window_property_and_type (Window  window,
                              Atom    atom, 
                              size_t  maxsize,
                              long   *length, 
                              Atom   *type, 
                              int    *size)
{
    if (window == 0x0) return NULL;

    unsigned char *prop = NULL;

    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    int status = XGetWindowProperty (
        __display, 
        window, 
        atom,
        0,
        maxsize,
        False, 
        AnyPropertyType, 
        &actual_type,
        &actual_format, 
        &nitems, 
        &bytes_after,
        &prop
    );

    if (status == BadWindow) {
        fprintf (stderr, "window id # 0x%lx does not exist!", __root_window);
        return NULL;
    }

    if (status != Success) {
        fprintf (stderr, "XGetWindowProperty failed!");
        return NULL;
    }

    unsigned long nbytes;
    switch (actual_format) {
        case 32:
            nbytes = sizeof (long);
            break;
        case 16:
            nbytes = sizeof (short);
            break;
        case 8:
            nbytes = 1;
            break;
        default:
            nbytes = 0;
            break;
    }

    *length = MIN (nitems * nbytes, ICON_BUFSIZE);
    *type = actual_type;
    *size = nbytes;
    return prop;
}

void 
apply_rotation (cairo_t *cr)
{
    // Rotate 90 deg
    cairo_translate (cr, ICON_WIDTH / 2, ICON_HEIGHT / 2);
    cairo_rotate (cr, M_PI_2);
    cairo_translate (cr, -ICON_WIDTH / 2, -ICON_HEIGHT / 2);
}

cairo_surface_t*
create_surface_for_xicon (unsigned long *icon,
                          size_t         icon_len)
{
    unsigned int idx = 0;
    unsigned int width = icon[idx++];
    unsigned int height = icon[idx++];

    // Reuse existing buffer
    int *pixel_buffer = (int *)&icon[idx];
    for (unsigned i = 0; i < MIN(width * height, icon_len); i++) {
        int pixel = icon[idx + i];
        
        int alpha = (pixel & 0xFF000000) >> 24;
        int red   = (pixel & 0x00FF0000) >> 16;
        int green = (pixel & 0x0000FF00) >> 8;
        int blue  = (pixel & 0x000000FF);

        float alphaFactor = (float)alpha / 255;
        red *= alphaFactor;
        green *= alphaFactor;
        blue *= alphaFactor;
        pixel_buffer[i] = (pixel & 0xFF000000) + (red << 16) + (green << 8) + blue;
    }

    const int stride = width * sizeof (int);
    cairo_surface_t *icon_surface = cairo_image_surface_create_for_data (
        (unsigned char *)pixel_buffer,
        CAIRO_FORMAT_ARGB32,
        width,
        height,
        stride
    );

    return icon_surface;
}

static void
compute_average_color (cairo_surface_t *pixmap,
                       double *out_red,
                       double *out_green,
                       double *out_blue)
{
    // This algorithm doesn't really produce the expected result, but I think
    // it looks quite good anyway.

    double r, g, b;
    double total_pixels = 0;
    
    int width = cairo_image_surface_get_width (pixmap);
    int height = cairo_image_surface_get_height (pixmap);

    const int stride = cairo_image_surface_get_stride (pixmap);
    unsigned char *surface_data = cairo_image_surface_get_data (pixmap);
    for (unsigned int row = 0; row < height; row++) {
        for (unsigned int col = 0; col < width; col++) {
            unsigned char *pixel = surface_data + (row * stride) + (width - col) * 4;
            char p_r = (pixel[0] & 0xFF);
            char p_g = (pixel[1] & 0xFF);
            char p_b = (pixel[2] & 0xFF);

            if (p_r > 10 && p_g > 10 && p_b > 10) {
                r += (p_r / 255.0);
                g += (p_g / 255.0);
                b += (p_b / 255.0);

                total_pixels++;
            }
        }
    }

    *out_red = (r / total_pixels);
    *out_green = (g / total_pixels);
    *out_blue = (b / total_pixels);
}

static void
draw_key (application_t *app, 
          infkey_t       key,
          infdevice_t   *device)
{

    cairo_surface_t *pixmap_surface = infpixmap_create_surface ();
    cairo_t *cr = cairo_create (pixmap_surface);
    apply_rotation (cr);

    cairo_surface_t *icon_surface = app->icon_surface;

    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    if (app->window == __active_window) {
        double r, g, b;
        if (icon_surface != NULL) {
            compute_average_color (icon_surface, &r, &g, &b);
        }

        cairo_set_source_rgb (cr, r, g, b);
    }

    cairo_paint (cr);

    if (icon_surface != NULL) {
        const unsigned int kMaxIconSize = 48;
        int width = cairo_image_surface_get_width (icon_surface);
        double scale_factor = ((double)kMaxIconSize / (double)width);
        cairo_scale (cr, scale_factor, scale_factor);

        // Center
        double center = (1 / scale_factor) * ((ICON_WIDTH - kMaxIconSize) / 2);

        cairo_set_source_surface (cr, icon_surface, center, center);
        cairo_paint (cr);
    }

    infpixmap_t *pixmap = infpixmap_create ();
    infpixmap_update_with_surface (pixmap, pixmap_surface);
    infdevice_set_pixmap_for_key_id (device, key, pixmap);

    infpixmap_free (pixmap);
    cairo_surface_destroy (pixmap_surface);
    cairo_destroy (cr);
}

void
clear_key (infkey_t     key,
           infdevice_t *device)
{
    infpixmap_t *pixmap = infpixmap_create ();
    infdevice_set_pixmap_for_key_id (device, key, pixmap);

    infpixmap_free (pixmap);
}

void
refresh_running_apps ()
{
    Atom type;
    int item_size;
    long int length;
    Window *windows = (Window *) get_window_property_and_type (
        __root_window,
        __a_client_list,
        TITLE_BUFSIZE,
        &length,
        &type,
        &item_size
    );

    // Items are Window pointers
    unsigned nitems = MIN(INF_NUM_KEYS, length / item_size);

    unsigned index = 0;
    for (unsigned i = 0; i < nitems; i++) {
        int propsize;
        long int proplen;
        unsigned char *title = get_window_property_and_type (
            windows[i],
            __a_wm_name,
            TITLE_BUFSIZE,
            &proplen,
            &type,
            &propsize
        );

        unsigned long *icon = (unsigned long *) get_window_property_and_type (
            windows[i],
            __a_wm_icon,
            ICON_BUFSIZE,
            &proplen,
            &type,
            &propsize
        );

        cairo_surface_t *icon_surface = NULL;
        if (icon != NULL) {
            icon_surface = create_surface_for_xicon (icon, proplen);
        } else {
            // RULE: Skip empty icons, for now. 
            continue;
        }

        // TODO: move these cleanup methods into someplace more sane
        if (__apps_for_keys[index].icon_surface != NULL) {
            XFree (__apps_for_keys[index].icon_surface_data);
            cairo_surface_destroy (__apps_for_keys[index].icon_surface);
        }

        if (__apps_for_keys[index].title != NULL) {
            XFree (__apps_for_keys[index].title);
        }

        __apps_for_keys[index] = (application_t) {
            .title = (char *)title,
            .window = windows[i],
            .icon_surface = icon_surface,
            .icon_surface_data = icon
        };

        index++;
    }

    XFree (windows);
    __num_running_apps = index;
}

static void 
draw_running_app_icons (infdevice_t *device)
{
    for (unsigned int i = 0; i < INF_NUM_KEYS; i++) {
        infkey_t key = infkey_num_to_key (horiz_key_order (i));

        if (i < __num_running_apps) {
            application_t app = __apps_for_keys[i];
            draw_key (&app, key, device);
        } else {
            clear_key (key, device);
        }
    }
}

static int
xlib_error_handler (Display *display, XErrorEvent *e)
{
    fprintf (stderr, "XLib error: %d\n", e->error_code);
    return 0;
}

static void*
input_handler_main (void *ctxt)
{
    infdevice_t *device = (infdevice_t *)ctxt;

    while (__running) {
        infkey_t pressed_key = infdevice_read_key (device);

        int keynum = infkey_to_key_num (pressed_key);
        int app_index = from_horiz_key_order (keynum);
        if (app_index < __num_running_apps) {
            application_t pressed_app = __apps_for_keys[app_index];
            raise_window_id (pressed_app.window);
        }
    }

    return NULL;
}

static void
handle_x_event (XEvent event)
{
    // Get active window
    Atom type;
    int item_size;
    long int length;
    Window *active_windows = (Window *) get_window_property_and_type (
        __root_window,
        __a_active_window,
        TITLE_BUFSIZE,
        &length,
        &type,
        &item_size
    );

    unsigned nitems = (length / item_size);
    __active_window = (nitems > 0 && active_windows != NULL) ? active_windows[0] : 0;
}

static void
runloop (infdevice_t *device)
{
    static pthread_t input_handler_thread = { 0 };
    pthread_create (&input_handler_thread, NULL, input_handler_main, device);

    XEvent event;
    while (__running) {
        refresh_running_apps ();
        draw_running_app_icons (device);
        
        // Assume the next X event we get is a window raise/create/destroy event,
        // and just continue the loop to refresh the list of apps
        XNextEvent (__display, &event);
        handle_x_event (event);
    }

    pthread_join (input_handler_thread, NULL);
}

int main (int argc, char **argv)
{
    __display = XOpenDisplay (NULL);
    if (!__display) {
        fprintf (stderr, "Error opening display.\n");
        return 1;
    }

    __root_window = DefaultRootWindow(__display);
    if (!__root_window) {
        fprintf (stderr, "Error getting root window for display\n");
        return 1;
    }

    infdevice_t *device = infdevice_open ();
    if (!device) {
        fprintf (stderr, "Error opening inf device\n");
        return 1;
    }

    // Atoms
    __a_active_window = XInternAtom (__display, "_NET_ACTIVE_WINDOW", False);
    __a_client_list = XInternAtom (__display, "_NET_CLIENT_LIST", True);
    __a_wm_name = XInternAtom (__display, "WM_NAME", True);
    __a_wm_icon = XInternAtom (__display, "_NET_WM_ICON", True);

    // Register for all X events
    XSelectInput (__display, __root_window, SubstructureNotifyMask);
    XSetErrorHandler (xlib_error_handler);

    runloop (device);

    infdevice_close (device);

    return 0;
}
