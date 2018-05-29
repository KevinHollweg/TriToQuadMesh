#pragma once

/*
 * PNG reading and writing functions using libpng.
 * http://www.libpng.org/
 *
 * credits for write PNG:
 * http://www.libpng.org/pub/png/book/chapter15.html
 * credits for read PNG:
 * http://www.libpng.org/pub/png/book/chapter13.html
 * http://blog.nobel-joergensen.com/2010/11/07/loading-a-png-as-texture-in-opengl-using-libpng/
 */

#include <vector>
#include <string>

namespace PNG{


struct PngImage{
    //image size
    int width, height;
    //number of bytes between the start of each row
    int pitchBytes;

    //number of bits per channel
    int bit_depth;
    //number of channels. for example RGB has 3 channels
    int channels;

    //one of the following:
    //PNG_COLOR_TYPE_GRAY,PNG_COLOR_TYPE_GRAY_ALPHA,PNG_COLOR_TYPE_RGB, PNG_COLOR_TYPE_RGB_ALPHA
    int color_type;

    //raw image data
    std::vector<unsigned char> data;
};

//prints the lib png version
void pngVersionInfo();

/**
 * Reads the png image from path and stores it in img.
 * Returns true if this operation is successful.
 * invertY: flips the rows of the image
 * alignment: minimum byte alignment for each row. Note: pitchBytes % alignment == 0
 */
bool readPNG(PngImage& img, const std::string &path, bool invertY = true, int alignment = 4);

/**
 * Saves the png image at path.
 */
bool writePNG(PngImage& img, const std::string &path, bool invertY = true);


}
