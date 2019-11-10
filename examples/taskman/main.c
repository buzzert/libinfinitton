#include <infinitton/infinitton.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <sys/param.h> // max, min

#define MAX_STR 500000
#define WINDOW(win) (long unsigned int *)win

Display *__display = NULL;
Window *__root_window = NULL;

static const char *
get_window_property_and_type (Atom  atom, 
                              long *length, 
                              Atom *type, 
                              int  *size)
{
    static unsigned char *prop = NULL;
    if (prop) {
        XFree (prop);
        prop = NULL;
    }

    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    int status = XGetWindowProperty (
            __display, 
            WINDOW(__root_window), 
            atom,
            0,
            (MAX_STR + 3) / 4,
			False, 
            AnyPropertyType, 
            &actual_type,
			&actual_format, 
            &nitems, 
            &bytes_after,
			&prop
    );

    if (status == BadWindow) {
        fprintf (stderr, "window id # 0x%lx does not exist!", WINDOW(__root_window));
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

    *length = MIN (nitems * nbytes, MAX_STR);
    *type = actual_type;
    *size = actual_format;
    return (const char *)prop;
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

    const char *result = get_window_property_and_type (

    return 0;
}
