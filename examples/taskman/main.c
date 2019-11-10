#include <infinitton/infinitton.h>

#include <assert.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <sys/param.h> // max, min

#define MAX_STR 500000
#define WINDOW(win) (long unsigned int)win

Display *__display = NULL;
Window *__root_window = NULL;

static const char *
get_window_property_and_type (Window *window,
                              Atom  atom, 
                              long *length, 
                              Atom *type, 
                              int  *size)
{
    unsigned char *prop = NULL;

#if 0 // callers must free!
    if (prop) {
        XFree (prop);
        prop = NULL;
    }
#endif

    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    int status = XGetWindowProperty (
            __display, 
            WINDOW(window), 
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
    *size = nbytes; // actual_format;
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

    Atom atom = XInternAtom (__display, "_NET_CLIENT_LIST", True);
    if (atom == BadAtom) {
        fprintf (stderr, "Bad atom.\n");
        return 1;
    }

    Atom type;
    int item_size, length;
    const char *result = get_window_property_and_type (
        __root_window,
        atom,
        &length,
        &type,
        &item_size
    );

    // Items are Window pointers
    unsigned nitems = (length / item_size);
    printf ("num windows: %d\n", nitems);

    Atom name_atom = XInternAtom (__display, "WM_NAME", True);
    Atom icon_atom = XInternAtom (__display, "_NET_WM_ICON", True);

    unsigned long int *windows = (unsigned long int *)result;
    for (unsigned i = 0; i < nitems; i++) {
        printf ("window id: 0x%x\n", windows[i]);
        
        int proplen, propsize;
        result = get_window_property_and_type (
                windows[i],
                name_atom,
                &proplen,
                &type,
                &propsize
        );

        printf("    name: %s\n", result);
    }

    return 0;
}
