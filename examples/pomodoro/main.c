#include <infinitton/infinitton.h>
#include <stdio.h>

static infdevice_t *g_shared_device;
static infpixmap_t *g_shared_pixmap;
static cairo_surface_t *g_shared_surface;

void initialize_drawing (void)
{
    g_shared_pixmap = infpixmap_create ();
    g_shared_surface = infpixmap_create_surface ();
}

void runloop (void)
{
}

int main (int argc, char **argv)
{
    g_shared_device = infdevice_open ();
    if (!g_shared_device) {
        fprintf(stderr, "Could not open device\n");
        return 1;
    }

    runloop ();

    infdevice_close (g_shared_device);
    return 0;
}

