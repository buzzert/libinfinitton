/*
 * pixmap.h
 *
 * Created 2019-02-13 by James Magahern <james@magahern.com>
 */

#pragma once

#include <stdlib.h>

struct infpixmap_t_;
typedef struct infpixmap_t_ infpixmap_t;

// Load a pixmap from a BMP file path
// The BMP file should be 72x72, 24-bits (R8 G8 B8), no colorspace info
extern infpixmap_t* infpixmap_open_file (const char *file_path);

// Get the data pointer
extern unsigned char* infpixmap_get_data (infpixmap_t *pixmap, size_t *out_length);

// Free/cleanup pixmap
extern void infpixmap_free (infpixmap_t *pixmap);

