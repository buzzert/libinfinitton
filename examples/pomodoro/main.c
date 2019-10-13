#include <infinitton/infinitton.h>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

static infdevice_t *g_shared_device;
static infpixmap_t *g_shared_pixmap;
static cairo_surface_t *g_shared_surface;
static PangoLayout *g_shared_layout;

static const unsigned int kDefaultTimerLengthSeconds = 3 * 60;
static const double kButtonSize = 30.0;

typedef enum square_role_t {
    TIMER_MIN = 0,
    TIMER_SEC = 5, 
    TIMER_PIE = 10, 

    BLINKY_SQUARE = 1,
    PLAY_PAUSE_BUTTON = 6,
    STOP_BUTTON = 11,
} SquareRole;

static struct {
    bool       exited;
    bool       running;

    SquareRole dirty_squares[15];
    size_t     num_dirty_squares;

    struct {
        unsigned int length;
        unsigned int remaining;

        bool flash_on;
        PangoFontDescription *min_font;
        PangoFontDescription *sec_font;
    } timer;
} g_app_state;

void apply_rotation (cairo_t *cr)
{
    // Rotate 90 deg
    cairo_translate (cr, ICON_WIDTH / 2, ICON_HEIGHT / 2);
    cairo_rotate (cr, M_PI_2);
    cairo_translate (cr, -ICON_WIDTH / 2, -ICON_HEIGHT / 2);
}

void update_pixmap_for_role (SquareRole role)
{
    infpixmap_update_with_surface (g_shared_pixmap, g_shared_surface);

    infkey_t key = infkey_num_to_key (role);
    infdevice_set_pixmap_for_key_id (g_shared_device, key, g_shared_pixmap);
}

void draw_string (cairo_t *cr, PangoFontDescription *font, const char *str)
{
    pango_layout_set_font_description (g_shared_layout, font);
    pango_layout_set_text (g_shared_layout, str, -1);

    int width, height;
    pango_layout_get_pixel_size (g_shared_layout, &width, &height);
    cairo_move_to (cr, (ICON_WIDTH - width) / 2.0, (ICON_HEIGHT - height) / 2.0);
    pango_cairo_show_layout (cr, g_shared_layout);
    cairo_fill (cr);
}

void draw_blinky_square (SquareRole role, cairo_t *cr)
{
    if (g_app_state.timer.flash_on) {
        cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    } else {
        cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    }

    cairo_paint (cr);
}

void draw_timer (SquareRole role, cairo_t *cr)
{
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

    // Calc minutes remaining
    int minutes_remain = g_app_state.timer.remaining / 60;

    enum { MINSTR_SIZE = 8 };
    char minutes_str[MINSTR_SIZE];
    snprintf (minutes_str, MINSTR_SIZE, "%d", minutes_remain);
    draw_string (cr, g_app_state.timer.min_font, minutes_str);
}

void draw_timer_sec (SquareRole role, cairo_t *cr)
{
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

    // Calc seconds remaining
    int seconds_remain = g_app_state.timer.remaining % 60;

    enum { SECSTR_SIZE = 8 };
    char seconds_str[SECSTR_SIZE];
    snprintf (seconds_str, SECSTR_SIZE, "%d", seconds_remain);
    draw_string (cr, g_app_state.timer.sec_font, seconds_str);
}

void draw_timer_pie (SquareRole role, cairo_t *cr)
{
    cairo_set_source_rgb (cr, 1.0, 0.6, 0.0);
    cairo_set_line_width (cr, 10.0);

    const double width = ICON_WIDTH;
    const double height = ICON_HEIGHT;

    double time_ratio = ((double)g_app_state.timer.remaining / (double)g_app_state.timer.length);

    cairo_translate (cr, width / 2, height / 2);
    cairo_rotate (cr, -M_PI_2);
    cairo_arc (cr, 0, 0, (width / 2) - 10.0, 0, (2 * M_PI) * time_ratio);
    cairo_stroke (cr);
}

void draw_play_pause_button (SquareRole role, cairo_t *cr)
{
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

    const double w = kButtonSize;
    const double h = kButtonSize;
    cairo_translate (cr, (ICON_WIDTH - w) / 2, (ICON_HEIGHT - h) / 2);

    if (g_app_state.running) {
        // Draw pause button
        cairo_rectangle (cr, 0.0, 0.0, w / 3, h);
        cairo_rectangle (cr, (w / 3) * 2, 0.0, w / 3, h);
        cairo_fill (cr);
    } else {
        // Draw play button
        cairo_move_to (cr, 0.0, 0.0);
        cairo_line_to (cr, 1.0 * w, 0.5 * w);
        cairo_line_to (cr, 0.0, 1.0 * h);
        cairo_line_to (cr, 0.0, 0.0);
        cairo_fill (cr);
    }
}

void draw_stop_button (SquareRole role, cairo_t *cr)
{
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

    const double w = kButtonSize;
    const double h = kButtonSize;
    cairo_translate (cr, (ICON_WIDTH - w) / 2, (ICON_HEIGHT - h) / 2);
    cairo_rectangle (cr, 0.0, 0.0, w, h);
    cairo_fill (cr);
}

void draw_dirty_squares (cairo_t *cr)
{
    cairo_set_source_rgb (cr, 0, 0, 0);
    for (unsigned int i = 0; i < g_app_state.num_dirty_squares; i++) {
        // Clear
        cairo_paint (cr);

        cairo_save (cr);
        SquareRole role = g_app_state.dirty_squares[i];
        switch (role) {
            case TIMER_MIN: draw_timer (role, cr); break;
            case TIMER_SEC: draw_timer_sec (role, cr); break;
            case TIMER_PIE: draw_timer_pie (role, cr); break;
            case PLAY_PAUSE_BUTTON: draw_play_pause_button (role, cr); break;
            case STOP_BUTTON: draw_stop_button (role, cr); break;
            case BLINKY_SQUARE: draw_blinky_square (role, cr); break;
            default: break;
        }

        update_pixmap_for_role (role);
        cairo_restore (cr);
    }

    g_app_state.num_dirty_squares = 0;
}

void mark_square_dirty (SquareRole square)
{
    g_app_state.dirty_squares[g_app_state.num_dirty_squares++] = square;
}

void initialize_drawing (void)
{
    g_shared_pixmap = infpixmap_create ();
    g_shared_surface = infpixmap_create_surface ();
    g_app_state.num_dirty_squares = 0;

    // Fonts 
    g_app_state.timer.min_font = pango_font_description_from_string ("Sans Bold 24");
    g_app_state.timer.sec_font = pango_font_description_from_string ("Sans 24");

    mark_square_dirty (PLAY_PAUSE_BUTTON);
    mark_square_dirty (STOP_BUTTON);
}

void runloop (void)
{
    cairo_t *cr = cairo_create (g_shared_surface);
    g_shared_layout = pango_cairo_create_layout (cr);
    apply_rotation (cr);

    while (!g_app_state.exited) {
        draw_dirty_squares (cr);

        sleep (1);

        if (g_app_state.running) {
            g_app_state.timer.flash_on = !g_app_state.timer.flash_on;
            g_app_state.timer.remaining--;
            mark_square_dirty (TIMER_MIN);
            mark_square_dirty (TIMER_SEC);
            mark_square_dirty (TIMER_PIE);
            mark_square_dirty (BLINKY_SQUARE);
        }
    }

    cairo_destroy (cr);
}

int main (int argc, char **argv)
{
    g_shared_device = infdevice_open ();
    if (!g_shared_device) {
        fprintf(stderr, "Could not open device\n");
        return 1;
    }

    initialize_drawing ();

    g_app_state.timer.length = kDefaultTimerLengthSeconds;
    g_app_state.timer.remaining = g_app_state.timer.length;
    g_app_state.running = true;

    runloop ();

    infdevice_close (g_shared_device);
    return 0;
}

