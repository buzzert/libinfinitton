/*
 * pixmap.c
 *
 * Created 2019-02-13 by James Magahern <james@magahern.com>
 */

#include "../include/pixmap.h"

#include <stdio.h>
#include <sys/stat.h>

struct infpixmap_t_ {
    unsigned char *data;
    size_t         size;
};

infpixmap_t* infpixmap_open_file (const char *file_path)
{
    // Obtain size of BMP
    struct stat st_buf;
    int status = stat (file_path, &st_buf);
    if (status != 0) {
        fprintf (stderr, "Could not stat bmp file\n");
        return NULL;
    }

    const unsigned long size = st_buf.st_size;

    FILE *f = fopen (file_path, "r");
    if (f == NULL) {
        fprintf (stderr, "Couldnt open bmp file for reading\n");
        return 0;
    }

    // Read the whole thing into memory, should be small
    unsigned char *buf = (unsigned char *)malloc (size);
    size_t read = fread(buf, size, 1, f);
    fclose(f);

    if (read <= 0) {
        fprintf (stderr, "Got no bytes\n");
        return 0;
    }

    struct infpixmap_t_ *pixmap = (struct infpixmap_t_ *)malloc (sizeof (struct infpixmap_t_));
    pixmap->data = buf;
    pixmap->size = size;

    return pixmap;
}

unsigned char* infpixmap_get_data (infpixmap_t *pixmap, size_t *out_length)
{
    *out_length = pixmap->size;
    return pixmap->data;
}

void infpixmap_free (infpixmap_t *pixmap)
{
    free (pixmap->data);
    free (pixmap);
}

