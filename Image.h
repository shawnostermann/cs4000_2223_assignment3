#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

enum allocation_type {
    NO_ALLOCATION, SELF_ALLOCATED, STB_ALLOCATED
};

typedef struct {
    int width;
    int height;
    int channels;
    size_t size;
    uint8_t *data;
    enum allocation_type allocation_;
} Image;

void Image_load(Image *img, const char *fname);
void Image_create(Image *img, int width, int height, int channels, bool zeroed);
void Image_save(const Image *img, const char *fname);
void Image_free(Image *img);
void Image_to_gray(const Image *orig, Image *gray);
void Image_to_sepia(const Image *orig, Image *sepia);

// Ostermann's routines
// return the pointer to the 8-bit data in the image at that position
u_int8_t *Img_get_pointer(Image *img, int channel, int column, int row);

// return the pixel on the channel at [column,row]
u_int8_t Img_pixel_read(Image *img, int channel, int column, int row);

// write the pixel on the channel at [column,row]
void Img_pixel_write(Image *img, int channel, int column, int row, int pixel);

// for debugging!!!
void Image_dump(Image *img);