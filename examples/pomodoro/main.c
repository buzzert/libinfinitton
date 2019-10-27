#include <infinitton/infinitton.h>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

static infdevice_t *g_shared_device;
static infpixmap_t *g_shared_pixmap;
static cairo_surface_t *g_shared_surface;
static PangoLayout *g_shared_layout;

static const unsigned int kDefaultTimerLengthSeconds = 25 * 60; // 25 minutes
static const double kButtonSize = 30.0;
static const double kDownBorderSize = 15.0;

typedef enum square_role_t {
    TIMER_MIN = 0,
    TIMER_SEC = 5, 
    TIMER_PIE = 10, 

    PLAY_PAUSE_BUTTON = 6,
    STOP_BUTTON = 11,

    SUB_MINUTE = 7,
    ADD_MINUTE = 12,

    INVALID = -1,
} SquareRole;

typedef enum event_type_t {
    PLAY_PAUSE_PRESSED,
    STOP_PRESSED,
    TICK,

    ADD_MINUTE_PRESSED,
    SUB_MINUTE_PRESSED,

    BUTTON_DOWN,
    BUTTON_UP,
} EventType; 

enum { MAX_EVENTS = 8 };

static struct {
    bool       exited;
    bool       running;

    SquareRole dirty_squares[15];
    size_t     num_dirty_squares;

    EventType  events[MAX_EVENTS];
    size_t     num_events;
    sem_t      post_event_sem;

    SquareRole button_down;
    SquareRole last_button_down;
    int        last_down_time;

    struct {
        unsigned int length;
        unsigned int remaining;

        bool flash_on;
        PangoFontDescription *min_font;
        PangoFontDescription *sec_font;
    } timer;
} g_app_state;

void push_event (EventType event)
{
    g_app_state.events[g_app_state.num_events++] = event;
    sem_post (&g_app_state.post_event_sem);
}

EventType pop_event (void)
{
    sem_wait (&g_app_state.post_event_sem);
    return g_app_state.events[--g_app_state.num_events];
}

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

void mark_square_dirty (SquareRole square)
{
    g_app_state.dirty_squares[g_app_state.num_dirty_squares++] = square;
}

void change_minute_pressed (int minute_amount)
{
    if (g_app_state.running)
        return; // these buttons have no effect while timer is running

    int delta = 60 * minute_amount;
    if (g_app_state.timer.length + delta > 0) {
        g_app_state.timer.length += 60 * minute_amount;
        g_app_state.timer.remaining = g_app_state.timer.length;
        mark_square_dirty (TIMER_MIN);
        mark_square_dirty (TIMER_SEC);
    }
}

bool role_is_interactive (SquareRole role)
{
    bool interactive = false;
    switch (role) {
        // Only draw for interactive elements
        case SUB_MINUTE:
        case ADD_MINUTE:
            // These are only interactive while NOT running
            if (g_app_state.running) break;

            // fallthrough
        case PLAY_PAUSE_BUTTON:
        case STOP_BUTTON:
            interactive = true;
            break;
        default:
            break;
    }

    return interactive;
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
    snprintf (seconds_str, SECSTR_SIZE, "%.2d", seconds_remain);
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

    if (g_app_state.timer.flash_on) {
        cairo_arc (cr, 0, 0, (width / 2) - 20.0, 0, 2 * M_PI);
        cairo_fill (cr);
    }
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

void draw_length_control_button (SquareRole role, cairo_t *cr)
{
    if (g_app_state.running)
        return; // don't draw these controls when timer is running

    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

    if (ADD_MINUTE == role) {
        draw_string (cr, g_app_state.timer.sec_font, "+");
    } else if (SUB_MINUTE == role) {
        draw_string (cr, g_app_state.timer.sec_font, "-");
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

void draw_down_button_indicator (SquareRole role, cairo_t *cr)
{
    if (role_is_interactive (role)) {
        cairo_save (cr);
        cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
        cairo_rectangle (cr, 0, 0, ICON_WIDTH, ICON_HEIGHT);
        cairo_set_line_width (cr, kDownBorderSize);
        cairo_stroke (cr);
        cairo_restore (cr);
    }
}

void draw_dirty_squares (cairo_t *cr)
{
    cairo_set_source_rgb (cr, 0, 0, 0);
    for (unsigned int i = 0; i < g_app_state.num_dirty_squares; i++) {
        // Clear
        cairo_paint (cr);

        SquareRole role = g_app_state.dirty_squares[i];

        // Button down state
        if (g_app_state.button_down == role) {
            draw_down_button_indicator (role, cr);
        }

        cairo_save (cr);
        switch (role) {
            case TIMER_MIN: draw_timer (role, cr); break;
            case TIMER_SEC: draw_timer_sec (role, cr); break;
            case TIMER_PIE: draw_timer_pie (role, cr); break;
            case PLAY_PAUSE_BUTTON: draw_play_pause_button (role, cr); break;
            case STOP_BUTTON: draw_stop_button (role, cr); break;
            case SUB_MINUTE: draw_length_control_button (role, cr); break;
            case ADD_MINUTE: draw_length_control_button (role, cr); break;
            default: break;
        }

        update_pixmap_for_role (role);
        cairo_restore (cr);
    }

    g_app_state.num_dirty_squares = 0;
}

void initialize_drawing (void)
{
    g_shared_pixmap = infpixmap_create ();
    g_shared_surface = infpixmap_create_surface ();
    g_app_state.num_dirty_squares = 0;

    // Fonts 
    g_app_state.timer.min_font = pango_font_description_from_string ("Sans Bold 24");
    g_app_state.timer.sec_font = pango_font_description_from_string ("Sans 24");

    mark_square_dirty (TIMER_MIN);
    mark_square_dirty (TIMER_SEC);
    mark_square_dirty (TIMER_PIE);
    mark_square_dirty (PLAY_PAUSE_BUTTON);
    mark_square_dirty (STOP_BUTTON);
    mark_square_dirty (ADD_MINUTE);
    mark_square_dirty (SUB_MINUTE);
}

void set_timer_running (bool running)
{
    g_app_state.running = running;
    mark_square_dirty (PLAY_PAUSE_BUTTON);
    mark_square_dirty (ADD_MINUTE);
    mark_square_dirty (SUB_MINUTE);
}

void reset_timer (void)
{
    g_app_state.timer.remaining = g_app_state.timer.length;

    mark_square_dirty (TIMER_MIN);
    mark_square_dirty (TIMER_SEC);
    mark_square_dirty (TIMER_PIE);
    mark_square_dirty (PLAY_PAUSE_BUTTON);
}

void on_tick (void)
{
    // Called on every "tick" (second)
    if (g_app_state.running) {
        g_app_state.timer.flash_on = !g_app_state.timer.flash_on;
        g_app_state.timer.remaining--;
        if (g_app_state.timer.remaining == 0) {
            // Stop timer
            set_timer_running (false);
        }

        mark_square_dirty (TIMER_MIN);
        mark_square_dirty (TIMER_SEC);
        mark_square_dirty (TIMER_PIE);
    }
}

int millitime (void)
{
    struct timespec tm;
    clock_gettime (CLOCK_MONOTONIC, &tm);

    int millis = (tm.tv_sec * 1000);
    millis += (tm.tv_nsec / 1000000);

    return millis;
}

void* input_handler (void *args)
{
    while (!g_app_state.exited) {
        infkey_t key = infdevice_read_key (g_shared_device);

        SquareRole role = infkey_to_key_num (key);
        g_app_state.button_down = role;
        g_app_state.last_down_time = millitime ();

        if (role == INVALID) {
            push_event (BUTTON_UP);
        } else {
            push_event (BUTTON_DOWN);
        }

        switch (role) {
            case PLAY_PAUSE_BUTTON: push_event (PLAY_PAUSE_PRESSED); break;
            case STOP_BUTTON: push_event (STOP_PRESSED); break;
            case ADD_MINUTE: push_event (ADD_MINUTE_PRESSED); break;
            case SUB_MINUTE: push_event (SUB_MINUTE_PRESSED); break;

            default: break;
        }
    }
    
    return NULL;
}

void* tick_handler (void *args)
{
    // TODO: better as a "green thread"?
    while (!g_app_state.exited) {
        push_event (TICK);

        int now = millitime ();

        // Allow add/sub minute buttons to repeat
        SquareRole down = g_app_state.button_down;
        int time_diff = (now - g_app_state.last_down_time);
        if ( (time_diff > 250) && (down == ADD_MINUTE || down == SUB_MINUTE)) {
            push_event ( (down == ADD_MINUTE) ? ADD_MINUTE_PRESSED : SUB_MINUTE_PRESSED );
            usleep (100000);
        } else {
            sleep (1);
        }
    }

    return NULL;
}

void runloop (void)
{
    cairo_t *cr = cairo_create (g_shared_surface);
    g_shared_layout = pango_cairo_create_layout (cr);
    apply_rotation (cr);

    while (!g_app_state.exited) {
        draw_dirty_squares (cr);

        EventType event = pop_event ();

        // Handle the event
        switch (event) {
            case TICK: 
                on_tick (); 
                break;

            case PLAY_PAUSE_PRESSED:
                set_timer_running (!g_app_state.running);
                if (g_app_state.timer.remaining == 0) {
                    reset_timer ();
                }

                break;

            case STOP_PRESSED:
                set_timer_running (false);
                reset_timer ();
                break;

            case ADD_MINUTE_PRESSED:
                change_minute_pressed (+1);
                break;

            case SUB_MINUTE_PRESSED:
                change_minute_pressed (-1);
                break;

            case BUTTON_UP:
                if (g_app_state.last_button_down != INVALID) {
                    mark_square_dirty (g_app_state.last_button_down);
                }

                break;
            case BUTTON_DOWN:
                g_app_state.last_button_down = g_app_state.button_down;
                mark_square_dirty (g_app_state.button_down);
                break;

            default: break;
        }
    }

    cairo_destroy (cr);
}

void print_usage (int argc, char **argv)
{
    fprintf (stderr, "Usage: %s [timer_length_minutes]\n", argv[0]);
    fprintf (stderr, "Optionally specify a length of time to run in minutes\n");
}

int main (int argc, char **argv)
{
    g_shared_device = infdevice_open ();
    if (!g_shared_device) {
        fprintf(stderr, "Could not open device\n");
        return 1;
    }

    g_app_state.timer.length = kDefaultTimerLengthSeconds;
    g_app_state.button_down = INVALID;
    g_app_state.last_button_down = INVALID;

    if (argc > 1) {
        char *length_str = argv[1];
        int length = atoi (length_str);
        if (length > 0) {
            g_app_state.timer.length = 60 * length;
        } else {
            fprintf (stderr, "Invalid timer length specified\n");
            print_usage (argc, argv);
        }
    }

    initialize_drawing ();

    sem_init (&g_app_state.post_event_sem, 0, 0);
    g_app_state.timer.remaining = g_app_state.timer.length;
    g_app_state.running = false;

    // Create input handler thread
    pthread_t input_thread, tick_thread;
    pthread_create (&input_thread, NULL, &input_handler, NULL);
    pthread_create (&tick_thread, NULL, &tick_handler, NULL);

    // Runs until exited = true
    runloop ();

    pthread_join (input_thread, NULL);
    pthread_join (tick_thread, NULL);

    infdevice_close (g_shared_device);
    return 0;
}

