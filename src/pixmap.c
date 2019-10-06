/*
 * pixmap.c
 *
 * Created 2019-02-13 by James Magahern <james@magahern.com>
 */

#include <infinitton/pixmap.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

struct infpixmap_t_ {
    unsigned char *data;
    size_t         size;

    int32_t        imgdata_offset;
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
    pixmap->imgdata_offset = -1; // TODO!!

    return pixmap;
}

infpixmap_t* infpixmap_create ()
{
    // Set up the BMP header
    struct __attribute__((__packed__)) {
        unsigned char header[2];
        int32_t size;

        unsigned char reserved[4];
        int32_t data_offset;
    } bmp_header;

    struct __attribute__((__packed__)) {
        int32_t header_size;
        int32_t width;
        int32_t height;
        
        int16_t num_color_planes;
        int16_t bpp; // bits per pixel
        
        int32_t comp_method;
        int32_t image_size;
        int32_t horiz_resolution;
        int32_t vert_resolution;
        int32_t num_colors; // number of colors in palette
        int32_t important_colors;
    } info_header;

    memset (&bmp_header, 0x00, sizeof (bmp_header));
    memset (&info_header, 0x00, sizeof (info_header));

    // BM
    bmp_header.header[0] = 'B'; bmp_header.header[1] = 'M';

    // Compute size of image data
    const size_t img_size = ICON_WIDTH * ICON_HEIGHT * 3; // 3 bytes per pixel
    bmp_header.size = img_size;

    // Populate info header
    info_header.header_size = sizeof (info_header);
    info_header.width = ICON_WIDTH;
    info_header.height = ICON_HEIGHT;
    info_header.num_color_planes = 1;
    info_header.bpp = 24;
    info_header.comp_method = 0; // BI_RGB
    info_header.image_size = img_size;
    info_header.horiz_resolution = 0; // ???
    info_header.vert_resolution = 0; // ???
    info_header.num_colors = 0;
    info_header.important_colors = 0;

    // Compute size of buffer
    const size_t header_size = sizeof (bmp_header) + sizeof (info_header);
    const size_t buf_size = header_size + img_size;
    bmp_header.data_offset = header_size;
    
    // Allocate data buffer
    unsigned char *data = (unsigned char *) calloc (buf_size, 1);

    // Copy header
    memcpy (data, &bmp_header, sizeof (bmp_header));
    memcpy (data + sizeof (bmp_header), &info_header, sizeof (info_header));

    // Create pixmap
    struct infpixmap_t_ *pixmap = (struct infpixmap_t_ *) malloc (sizeof (struct infpixmap_t_));
    pixmap->data = data;
    pixmap->size = buf_size;
    pixmap->imgdata_offset = bmp_header.data_offset;

    return pixmap;
}

cairo_surface_t* infpixmap_get_surface (infpixmap_t *pixmap)
{
    const int stride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, ICON_WIDTH);
    cairo_surface_t *surface = cairo_image_surface_create_for_data (
        pixmap->data + pixmap->imgdata_offset,
        CAIRO_FORMAT_RGB24,
        ICON_WIDTH,
        ICON_HEIGHT,
        stride
    );

    return surface;
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

/* Cairo Extensions */

cairo_surface_t* infpixmap_create_surface ()
{
    return cairo_image_surface_create (CAIRO_FORMAT_RGB24, ICON_WIDTH, ICON_HEIGHT);
}

void infpixmap_update_with_surface (infpixmap_t     *pixmap, 
                                    cairo_surface_t *surface)
{
    unsigned char *data = pixmap->data;

    // Convert surface data.
    // Cairo stores each pixel as a 32-bit quantity, where the upper 8-bits are unused.
    // For BMP, we need to convert pixels to 24-bit quantities
    unsigned int offset = pixmap->imgdata_offset;
    const int stride = cairo_image_surface_get_stride (surface);
    unsigned char *surface_data = cairo_image_surface_get_data (surface);
    for (unsigned int row = 0; row < ICON_HEIGHT; row++) {
        for (unsigned int col = 0; col < ICON_WIDTH; col++) {
            // 4 bytes per pixel in source data
            unsigned char *pixel = surface_data + (row * stride) + (ICON_WIDTH - col) * 4;
            data[offset++] = pixel[0] & 0xFF;
            data[offset++] = pixel[1] & 0xFF;
            data[offset++] = pixel[2] & 0xFF;
        }
    }
}
