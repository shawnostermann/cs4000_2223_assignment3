// Both sharpen and shrink an image
// Shawn Ostermann - Feb 23, 2023

#include <iostream>
#include <string>
#include "Image.h"
#include "utils.h"
#include <math.h>
using namespace std;

int debug = 0;

const int numchannels = 3;  // red, green, blue

////////////////////////////////////////////////////////////////////////////////
// several fun convolutional matrices to play with
////////////////////////////////////////////////////////////////////////////////

float matrix_identity[3][3] = 
    { { 0, 0, 0}, 
      { 0, 1, 0}, 
      { 0, 0, 0} };

float matrix_sharpen[3][3] = 
    { { 0, -1,  0}, 
      {-1,  5, -1}, 
      { 0, -1,  0} };

float matrix_outline[3][3] = 
    { {-1, -1, -1}, 
      {-1,  8, -1}, 
      {-1, -1, -1} };

float matrix_top_sobel[3][3] = 
    { { 1,  2,  1}, 
      { 0,  0,  0}, 
      {-1, -2, -1} };

float matrix_bottem_sobel[3][3] = 
    { {-1, -2, -1}, 
      { 0,  0,  0}, 
      { 1,  2,  1} };

float matrix_blur[3][3] = 
    { {0.0625, 0.1250, 0.0625}, 
      {0.1250, 0.2500, 0.1250}, 
      {0.0625, 0.1250, 0.0625} };



// apply the convolutional matrix to all 3 channels of that one pixel
void Update_pixel(Image *img_orig, Image *img_sharper, float matrix[3][3], int column, int row) {
    if (debug)
        printf("Updating pixel[%d][%d]\n", column, row);
    for (int channel=0; channel < numchannels; ++channel) {  // iterate over red, green, and blue channels
        float newpixel_float = 0;
        // apply the 3x3 convolution matrix
        for (int matrix_col=0; matrix_col<3; ++matrix_col) {
            for (int matrix_row=0; matrix_row<3; ++matrix_row) {
                int oldpixel = Img_pixel_read(img_orig, channel, column+(matrix_col-1), row+(matrix_row-1));
                if (debug)
                    printf(" oldpixel[%u,%d][%d] = %d\n", column+(matrix_col-1), row+(matrix_row-1), channel, oldpixel);
                newpixel_float += ((float)oldpixel * matrix[matrix_col][matrix_row]);
                if (debug)
                    printf("   matrix[%u,%u]=%f\n", matrix_col, matrix_row, matrix[matrix_col][matrix_row]);
            }
        }    

        // keep the pixel value within 8 bits
        if (newpixel_float < 0)
            newpixel_float = 0.0; 
        else if (newpixel_float > 255)
            newpixel_float = 255.0;

        int newpixel = (u_int8_t) newpixel_float;
        if (debug)
            printf("      final value: %f (%u)\n", newpixel_float, newpixel);
        Img_pixel_write(img_sharper, channel, column, row, newpixel);
    }
}

// create a new pixel in the img_smaller[newcolumn,newrow] that is the average of the corresponding (shrink x shrink)
// square in img_bigger
void Average_pixels(Image *img_bigger, Image *img_smaller, int shrink, int newcolumn, int newrow) {
    for (int channel=0; channel < numchannels; ++channel) {  // iterate over red, green, and blue channels
        float newpixel_float = 0;
        int oldcolumn = newcolumn * shrink;
        int oldrow    = newrow    * shrink;

        for (int col = 0; col < shrink; ++col) {
            for (int row = 0; row < shrink; ++row) {
                int oldpixel = Img_pixel_read(img_bigger, channel, oldcolumn+col, oldrow+row);
                newpixel_float += oldpixel;
            }
        }

        newpixel_float /= (shrink*shrink);  // convert to average
        
        // keep the pixel value within 8 bits
        if (newpixel_float < 0)
            newpixel_float = 0.0; 
        else if (newpixel_float > 255)
            newpixel_float = 255.0;

        int newpixel = (u_int8_t) newpixel_float;
        if (debug)
            printf("      final value: %f (%u)\n", newpixel_float, newpixel);
        Img_pixel_write(img_smaller, channel, newcolumn, newrow, newpixel);
    }
}


void Image_sharpen(Image *img_orig, Image *img_sharper, Image *img_smaller, int numthreads, int shrink) 
{
    if (debug) 
        Image_dump(img_orig);

    // create a new image to hold the sharpened version
    Image_create(img_sharper, img_orig->width, img_orig->height, img_orig->channels, false);
    ON_ERROR_EXIT(img_sharper->data == NULL, "Error in creating the image");

    // create a new image to hold the smaller version
    Image_create(img_smaller, img_orig->width/shrink, img_orig->height/shrink, img_orig->channels, false);
    ON_ERROR_EXIT(img_smaller->data == NULL, "Error in creating the image");

    fprintf(stderr,"Sharpening image...\n");

    // use the matrix to create a sharper copy of the original image
    // this part, at least, will need to be parallelized
    for (int column=0; column < (img_orig->width); ++column) {
        for (int row=0; row < (img_orig->height); ++row) {
            // the edges are a special case
            if ((column==0)||(row==0)||(column==img_orig->width-1)||(row==img_orig->height-1)) {
                for (int channel=0; channel < numchannels; ++channel) {  // iterate over red, green, and blue channels
                    int oldpixel = Img_pixel_read(img_orig, channel, column, row);
                    Img_pixel_write(img_sharper, channel, column, row, oldpixel);
                }
            } else {
                Update_pixel(img_orig, img_sharper, matrix_sharpen, column, row);
            }
        }
    }

    fprintf(stderr,"Shrinking image...\n");

    // shrink the image
    // This part could start before the sharpening finishes, but be careful
    for (int column=0; column < (img_orig->width/shrink); ++column) {
        for (int row=0; row < (img_orig->height/shrink); ++row) {
            Average_pixels(img_sharper, img_smaller, shrink, column, row);
        }
    }

    if (debug) 
        Image_dump(img_sharper);
}



int main(int argc, char *argv[]) 
{
    Image img_infile, img_sharper, img_smaller;

    if (argc != 4) {
        fprintf(stderr,"Requires 3 arguments: %s infile numthreads shrink\n", argv[0]);
        exit(1);
    }

    char *infile = argv[1];
    char outfile_sharper[] = "sharper.png";
    char outfile_smaller[] = "smaller.png";    
    int numthreads = atoi(argv[2]);
    int shrink = atoi(argv[3]);


    if (debug) {
        cout << "Infile: '" << infile << "'\n";
        cout << "Sharper: '" << outfile_sharper << "'\n";
        cout << "Smaller: '" << outfile_smaller << "'\n";
        cout << "Numthreads: " << numthreads << endl;
        cout << "Shrink: " << shrink << endl;
    }
    
    // load the requested image
    fprintf(stderr,"Loading image...\n");
    Image_load(&img_infile, infile);
    ON_ERROR_EXIT(img_infile.data == NULL, "Error in loading the image");
    ON_ERROR_EXIT(!(img_infile.allocation_ != NO_ALLOCATION && img_infile.channels == 3), "The input image must have exactly 3 channels.");
    
    // Converting the image
    fprintf(stderr,"Altering image...\n");
    Image_sharpen(&img_infile, &img_sharper, &img_smaller, numthreads, shrink);

    // Save image
    fprintf(stderr,"Saving images...\n");
    Image_save(&img_sharper, outfile_sharper);
    Image_save(&img_smaller, outfile_smaller);

    // Release memory
    Image_free(&img_infile);
    Image_free(&img_sharper);
    Image_free(&img_smaller);

    exit(0);
}
