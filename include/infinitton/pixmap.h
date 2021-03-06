/*
 * pixmap.h
 *
 * Created 2019-02-13 by James Magahern <james@magahern.com>
 */

#pragma once

#include <stdlib.h>
#include <cairo/cairo.h>

#define ICON_WIDTH  72
#define ICON_HEIGHT 72

struct infpixmap_t_;
typedef struct infpixmap_t_ infpixmap_t;

// Creates an empty infpixmap_t. 
extern infpixmap_t* infpixmap_create ();

// Creates a pixmap by loading a pixmap from a BMP file path
// The BMP file should be 72x72, 24-bits (R8 G8 B8), no colorspace info
extern infpixmap_t* infpixmap_open_file (const char *file_path);

// Get the raw data pointer, including the bmp header
extern unsigned char* infpixmap_get_data (infpixmap_t *pixmap, size_t *out_length);

// Returns a pointer to just the image data, stored as RGB, 24-bit quantities
extern unsigned char* infpixmap_get_image_data (infpixmap_t *pixmap, size_t *out_length);

// Free/cleanup pixmap
extern void infpixmap_free (infpixmap_t *pixmap);

/* Cairo Extensions */

// Convenience: Returns a properly configured cairo surface for drawing
extern cairo_surface_t* infpixmap_create_surface ();

// Updates a pixmap's pixel data with the given cairo surface in place. 
extern void infpixmap_update_with_surface (infpixmap_t     *pixmap, 
                                           cairo_surface_t *surface);

