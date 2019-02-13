#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <hidapi/hidapi.h>

typedef struct __attribute__((__packed__)) {
    int32_t offset;
    int32_t length; // 0x1f40

    int32_t ctrl1;  // 0x55aaaa55
    int32_t ctrl2;  // 0x44332211
} binary_data_partial_t;

typedef struct __attribute__((__packed__)) {
    int8_t  a; // 0x12
    int16_t b; // 0x1
    char    c;
    
    int32_t screen_num;

    int32_t d; // 0x0
    int32_t e; // 0x0
    int32_t number; // always 15606

    char    padding[13];
} feature_packet_t;

// Returns size
unsigned long transfer_bitmap(const char *bmp_path, hid_device *device)
{
    // Obtain size of BMP
    struct stat st_buf;
    int status = stat(bmp_path, &st_buf);
    if (status != 0) {
        fprintf(stderr, "Could not stat bmp file\n");
        return 0;
    }

    const unsigned long size = st_buf.st_size;

    FILE *f = fopen(bmp_path, "r");
    if (f == NULL) {
        fprintf(stderr, "Couldnt open bmp file for reading\n");
        return 0;
    }

    // Read the whole thing into memory, should be small
    unsigned char *buf = (unsigned char *)malloc(size);
    size_t read = fread(buf, size, 1, f);
    fclose(f);

    if (read <= 0) {
        fprintf(stderr, "Got no bytes\n");
        return 0;
    }

    // Malloc payload and copy data after the header
    const int8_t report_id = 0x02;
    const size_t payload_size = 8017; // sent in two 8017 byte chunks
    
    size_t remain = size;

    size_t offset = 0;
    void *payload = calloc(payload_size, 1);
    
    // Write report_id (0x02)
    memcpy(payload, &report_id, 1);
    offset += 1;

    // Write first header
    binary_data_partial_t header = {
        .offset = 0,
        .length = payload_size,
        .ctrl1  = 0x55aaaa55,
        .ctrl2  = 0x44332211
    };
    memcpy(payload + offset, &header, sizeof(header));
    offset += sizeof(header);

    // Write first half of image data
    memcpy(payload + offset, buf, payload_size - offset);
    remain -= payload_size - offset;

    // TRANSMIT
    hid_write(device, payload, payload_size);
    
    // Second payload
    memset(payload, 0, payload_size);

    // report_id
    memcpy(payload, &report_id, 1);
    offset = 1;
    
    // Second header
    header.offset = payload_size;
    header.length = sizeof(header) + remain;
    memcpy(payload + offset, &header, sizeof(header));
    offset += sizeof(header);

    // Second half of image data
    memcpy(payload + offset, buf + payload_size, remain);

    // TRANSMIT
    hid_write(device, payload, payload_size);

    // Cleanup
    free(payload);
    free(buf);
    
    return size;
}

void send_feature(int screen_num, unsigned long img_size, hid_device *device)
{
    feature_packet_t payload = {
        .a = 0x12, // 0x12
        .b = 0x01, // 0x1
        
        .screen_num = screen_num,

        .d = 0x0,      // 0x0
        .e = 0x0,      // 0x0
        .number = img_size
    };

    hid_send_feature_report(device, (const unsigned char *)&payload, sizeof(feature_packet_t));
    // fwrite(&payload, sizeof(feature_packet_t), 1, stdout);
}

void send_image(const char *bmp_path, int screen_num, hid_device *device)
{
    unsigned long img_len = transfer_bitmap(bmp_path, device);
    send_feature(screen_num, img_len, device);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s bmp_file\n", argv[0]);
        return 1;
    }

    hid_device *device = hid_open(0xffff, 0x1f40, NULL);
    if (device != NULL) {
        // Put the image on all of the screens
        for (unsigned int i = 1; i < 16; i++) {
            send_image(argv[1], i, device);   
        }
        hid_close(device);
    } else {
        fprintf(stderr, "Unable to open device: %s\n", (const char *)hid_error(device));
        fprintf(stderr, "Check permissions?\n");
        return 1;
    }

    return 0;
}

