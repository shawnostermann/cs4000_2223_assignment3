#include "Image.h"
#include "utils.h"
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

void Image_load(Image *img, const char *fname) {
    if((img->data = stbi_load(fname, &img->width, &img->height, &img->channels, 0)) != NULL) {
        img->size = img->width * img->height * img->channels;
        img->allocation_ = STB_ALLOCATED;
    }
}

void Image_create(Image *img, int width, int height, int channels, bool zeroed) {
    size_t size = width * height * channels;
    if(zeroed) {
        img->data = (uint8_t *) calloc(size, 1);
    } else {
        img->data = (uint8_t *) malloc(size);
    }

    if(img->data != NULL) {
        img->width = width;
        img->height = height;
        img->size = size;
        img->channels = channels;
        img->allocation_ = SELF_ALLOCATED;
    }
}

void Image_save(const Image *img, const char *fname) {
    // Check if the file name ends in one of the .jpg/.JPG/.jpeg/.JPEG or .png/.PNG
    if(str_ends_in(fname, ".jpg") || str_ends_in(fname, ".JPG") || str_ends_in(fname, ".jpeg") || str_ends_in(fname, ".JPEG")) {
        stbi_write_jpg(fname, img->width, img->height, img->channels, img->data, 100);
    } else if(str_ends_in(fname, ".png") || str_ends_in(fname, ".PNG")) {
        stbi_write_png(fname, img->width, img->height, img->channels, img->data, img->width * img->channels);
    } else {
        ON_ERROR_EXIT(false, "");
    }
}

void Image_free(Image *img) {
    if(img->allocation_ != NO_ALLOCATION && img->data != NULL) {
        if(img->allocation_ == STB_ALLOCATED) {
            stbi_image_free(img->data);
        } else {
            free(img->data);
        }
        img->data = NULL;
        img->width = 0;
        img->height = 0;
        img->size = 0;
        img->allocation_ = NO_ALLOCATION;
    }
}

void Image_to_gray(const Image *orig, Image *gray) {
    ON_ERROR_EXIT(!(orig->allocation_ != NO_ALLOCATION && orig->channels >= 3), "The input image must have at least 3 channels.");
    int channels = orig->channels == 4 ? 2 : 1;
    Image_create(gray, orig->width, orig->height, channels, false);
    ON_ERROR_EXIT(gray->data == NULL, "Error in creating the image");

    for(unsigned char *p = orig->data, *pg = gray->data; p != orig->data + orig->size; p += orig->channels, pg += gray->channels) {
        *pg = (uint8_t)((*p + *(p + 1) + *(p + 2))/3.0);
        if(orig->channels == 4) {
            *(pg + 1) = *(p + 3);
        }
    }
}

void Image_to_sepia(const Image *orig, Image *sepia) {
    ON_ERROR_EXIT(!(orig->allocation_ != NO_ALLOCATION && orig->channels >= 3), "The input image must have at least 3 channels.");
    Image_create(sepia, orig->width, orig->height, orig->channels, false);
    ON_ERROR_EXIT(sepia->data == NULL, "Error in creating the image");

    // Sepia filter coefficients from https://stackoverflow.com/questions/1061093/how-is-a-sepia-tone-created
    for(unsigned char *p = orig->data, *pg = sepia->data; p != orig->data + orig->size; p += orig->channels, pg += sepia->channels) {
        *pg       = (uint8_t)fmin(0.393 * *p + 0.769 * *(p + 1) + 0.189 * *(p + 2), 255.0);         // red
        *(pg + 1) = (uint8_t)fmin(0.349 * *p + 0.686 * *(p + 1) + 0.168 * *(p + 2), 255.0);         // green
        *(pg + 2) = (uint8_t)fmin(0.272 * *p + 0.534 * *(p + 1) + 0.131 * *(p + 2), 255.0);         // blue 
        if(orig->channels == 4) {
            *(pg + 3) = *(p + 3);
        }
    }
}


// Ostermann's support routines
static int debug=0;

// return the pointer to the 8-bit data in the image at that position
u_int8_t *Img_get_pointer(Image *img, int channel, int column, int row) {
    int channels = img->channels;
    u_int8_t *bits = img->data;
    u_int8_t *prow = bits + row*(img->width*channels);
    u_int8_t *pcol = prow + column*(channels);
    u_int8_t *ppixel = pcol + channel;
    return(ppixel);
}      

// return the pixel on the channel at [column,row]
u_int8_t Img_pixel_read(Image *img, int channel, int column, int row) {
    u_int8_t *ppixel = Img_get_pointer(img, channel, column, row);
    if (debug>9)
        printf("pixel[%u,%u][%u] = %3u (%p)\n", column, row, channel, *ppixel, ppixel);
    return(*ppixel);
}

// write the pixel on the channel at [column,row]
void Img_pixel_write(Image *img, int channel, int column, int row, int pixel) {
    u_int8_t *ppixel = Img_get_pointer(img, channel, column, row);
    *ppixel = pixel;
}


// for debugging!!!
void Image_dump(Image *img) {
    printf("Image is %dx%d with %d channels\n", img->width, img->height, img->channels);
    for (int row=0; row < (img->height); ++row) {
        for (int column=0; column < (img->width); ++column) {
            int red   = Img_pixel_read(img, 0, column, row);
            int green = Img_pixel_read(img, 1, column, row);
            int blue  = Img_pixel_read(img, 2, column, row);
            printf("(%3d,%3d,%3d) ", red, green, blue);
        }
        printf("\n");
    }
}


