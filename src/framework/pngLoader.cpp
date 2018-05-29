/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#include "pngLoader.h"

#include <png.h>
#include <cstring> // for memcpy
#include <assert.h>
#include <iostream>

namespace PNG{


using uchar = unsigned char;

//======================================= Helper Functions =====================================================

struct PNGHelperData
{
    uchar **row_pointers;
    void *png_ptr;
    void *info_ptr;
    FILE *infile;
    FILE *outfile;
    jmp_buf jmpbuf;
};

static void writepng_error_handler(png_structp png_ptr, png_const_charp msg)
{

    /* This function, aside from the extra step of retrieving the "error
     * pointer" (below) and the fact that it exists within the application
     * rather than within libpng, is essentially identical to libpng's
     * default error handler.  The second point is critical:  since both
     * setjmp() and longjmp() are called from the same code, they are
     * guaranteed to have compatible notions of how big a jmp_buf is,
     * regardless of whether _BSD_SOURCE or anything else has (or has not)
     * been defined. */

    fprintf(stderr, "writepng libpng error: %s\n", msg);
    fflush(stderr);

    PNGHelperData  *data = static_cast<PNGHelperData*>(png_get_error_ptr(png_ptr));
    if (data == NULL) {         /* we are completely hosed now */
        fprintf(stderr,
                "writepng severe error:  jmpbuf not recoverable; terminating.\n");
        fflush(stderr);
        assert(0);
    }

    longjmp(data->jmpbuf, 1);
}

static std::string colorTypeToString(int colorType)
{
    switch(colorType)
    {
    case PNG_COLOR_TYPE_GRAY:
        return "PNG_COLOR_TYPE_GRAY";
    case PNG_COLOR_TYPE_PALETTE:
        return "PNG_COLOR_TYPE_PALETTE";
    case PNG_COLOR_TYPE_RGB:
        return "PNG_COLOR_TYPE_RGB";
    case PNG_COLOR_TYPE_RGB_ALPHA:
        return "PNG_COLOR_TYPE_RGB_ALPHA";
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        return "PNG_COLOR_TYPE_GRAY_ALPHA";
    default:
        assert(0);
        return "UNKNOWN COLOR TYPE";
    }
}

static int channelsForColorType(int colorType)
{
    switch(colorType)
    {
    case PNG_COLOR_TYPE_GRAY:
        return 1;
    case PNG_COLOR_TYPE_PALETTE:
        return 1;
    case PNG_COLOR_TYPE_RGB:
        return 3;
    case PNG_COLOR_TYPE_RGB_ALPHA:
        return 4;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        return 2;
    default:
        assert(0);
        return 0;
    }
}

void pngVersionInfo()
{
    std::cout<<"libpng version: "<<png_libpng_ver<< std::endl;
}

inline
int iAlignUp(int a, int b) { return (a % b != 0) ?  (a - a % b + b) : a; }

//======================================= Read PNG =====================================================

bool readPNG(PngImage& img, const std::string &path, bool invertY, int alignment)
{
    png_structp png_ptr;
    png_infop info_ptr;
    PNGHelperData data;

    unsigned int sig_read = 0;
    int  interlace_type;

    if ((data.infile = fopen(path.c_str(), "rb")) == NULL)
        return false;

    /* Create and initialize the png_struct
     * with the desired error handler
     * functions.  If you want to use the
     * default stderr and longjump method,
     * you can supply NULL for the last
     * three parameters.  We also supply the
     * the compiler header file version, so
     * that we know if the application
     * was compiled with a compatible version
     * of the library.  REQUIRED
     */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                     NULL, NULL, NULL);

    if (png_ptr == NULL) {
        fclose(data.infile);
        return false;
    }

    /* Allocate/initialize the memory
     * for image information.  REQUIRED. */
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fclose(data.infile);
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return false;
    }

    /* Set error handling if you are
     * using the setjmp/longjmp method
     * (this is the normal method of
     * doing things with libpng).
     * REQUIRED unless you  set up
     * your own error handlers in
     * the png_create_read_struct()
     * earlier.
     */
    if (setjmp(png_jmpbuf(png_ptr))) {
        /* Free all of the memory associated
         * with the png_ptr and info_ptr */
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(data.infile);
        /* If we get here, we had a
         * problem reading the file */
        return false;
    }

    /* Set up the output control if
     * you are using standard C streams */
    png_init_io(png_ptr, data.infile);

    /* If we have already
     * read some of the signature */
    png_set_sig_bytes(png_ptr, sig_read);

    /*
     * If you have enough memory to read
     * in the entire image at once, and
     * you need to specify only
     * transforms that can be controlled
     * with one of the PNG_TRANSFORM_*
     * bits (this presently excludes
     * dithering, filling, setting
     * background, and doing gamma
     * adjustment), then you can read the
     * entire image (including pixels)
     * into the info structure with this
     * call
     *
     * PNG_TRANSFORM_STRIP_16 |
     * PNG_TRANSFORM_PACKING  forces 8 bit
     * PNG_TRANSFORM_EXPAND forces to
     *  expand a palette into RGB
     */
    png_read_png(png_ptr, info_ptr,
                 //                 PNG_TRANSFORM_STRIP_16 | //Strip 16-bit samples to 8 bits
                 PNG_TRANSFORM_PACKING | //Expand 1, 2 and 4-bit samples to bytes
                 PNG_TRANSFORM_EXPAND //Perform set_expand()
                 , NULL);



        png_uint_32 w,h;
    png_get_IHDR(png_ptr, info_ptr, &w, &h, &img.bit_depth, &img.color_type,
                 &interlace_type, NULL, NULL);
    img.width = w;
    img.height = h;


	unsigned int row_bytes = (unsigned int)png_get_rowbytes(png_ptr, info_ptr);

    //we want to row-align the image in our output data
//    int rowPadding = (img.rowAlignment - (row_bytes % img.rowAlignment)) % img.rowAlignment;
    img.pitchBytes = iAlignUp(row_bytes,alignment);

//    img.pitchBytes = png_get_rowbytes(png_ptr, info_ptr);

    img.data.resize(img.pitchBytes * img.height);

    png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);

    if(invertY){
        for (int i = 0; i < img.height; i++) {
            memcpy(img.data.data()+(img.pitchBytes * (img.height-1-i)), row_pointers[i], row_bytes);
        }
    }else{
        for (int i = 0; i < img.height; i++) {
            memcpy(img.data.data()+(img.pitchBytes * i), row_pointers[i], row_bytes);
        }
    }

    /* Clean up after the read,
     * and free any memory allocated */
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    /* Close the file */
    fclose(data.infile);

    img.channels = channelsForColorType(img.color_type);

    std::cout << "Read PNG success! " << path << " Size: " << img.width << "x" << img.height << " Type: " << img.channels << "x" << img.bit_depth << " " << colorTypeToString(img.color_type) << std::endl;

    return true;
}

//======================================= Write PNG =====================================================

/* returns 0 for success, 2 for libpng problem, 4 for out of memory, 11 for
 *  unexpected pnmtype; note that outfile might be stdout */
static int writepng_init(const PNG::PngImage& image,  PNGHelperData* data)
{
    png_structp  png_ptr;       /* note:  temporary variables! */
    png_infop  info_ptr;
    int  interlace_type;


    /* could also replace libpng warning-handler (final NULL), but no need: */

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, data,writepng_error_handler, NULL);
    if (!png_ptr)
        return 4;   /* out of memory */

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        return 4;   /* out of memory */
    }


    /* setjmp() must be called in every function that calls a PNG-writing
     * libpng function, unless an alternate error handler was installed--
     * but compatible error handlers must either use longjmp() themselves
     * (as in this program) or exit immediately, so here we go: */

    if (setjmp(data->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return 2;
    }


    /* make sure outfile is (re)opened in BINARY mode */

    png_init_io(png_ptr, data->outfile);


    /* set the compression levels--in general, always want to leave filtering
     * turned on (except for palette images) and allow all of the filters,
     * which is the default; want 32K zlib window, unless entire image buffer
     * is 16K or smaller (unknown here)--also the default; usually want max
     * compression (NOT the default); and remaining compression flags should
     * be left alone */

    // png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
    /*
    >> this is default for no filtering; Z_FILTERED is default otherwise:
    png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
    >> these are all defaults:
    png_set_compression_mem_level(png_ptr, 8);
    png_set_compression_window_bits(png_ptr, 15);
    png_set_compression_method(png_ptr, 8);
 */


    /* set the image parameters appropriately */

    interlace_type = PNG_INTERLACE_NONE; //PNG_INTERLACE_ADAM7

    png_set_IHDR(png_ptr, info_ptr, image.width, image.height,
                 image.bit_depth, image.color_type, interlace_type,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);


    /* write all chunks up to (but not including) first IDAT */
    png_write_info(png_ptr, info_ptr);


    /* if we wanted to write any more text info *after* the image data, we
     * would set up text struct(s) here and call png_set_text() again, with
     * just the new data; png_set_tIME() could also go here, but it would
     * have no effect since we already called it above (only one tIME chunk
     * allowed) */


    /* set up the transformations:  for now, just pack low-bit-depth pixels
     * into bytes (one, two or four pixels per byte) */

    png_set_packing(png_ptr);
    /*  png_set_shift(png_ptr, &sig_bit);  to scale low-bit-depth values */


    /* make sure we save our pointers for use in writepng_encode_image() */

    data->png_ptr = png_ptr;
    data->info_ptr = info_ptr;


    /* OK, that's all we need to do for now; return happy */
    return 0;
}

/* returns 0 for success, 2 for libpng (longjmp) problem */
static int writepng_encode_image(PNG::PngImage& image, PNGHelperData* data, bool invertY)
{
    png_structp png_ptr = (png_structp)data->png_ptr;
    png_infop info_ptr = (png_infop)data->info_ptr;

    /* as always, setjmp() must be called in every function that calls a
     * PNG-writing libpng function */
    if (setjmp(data->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        data->png_ptr = NULL;
        data->info_ptr = NULL;
        return 2;
    }

    std::vector<uchar*> row_pointers(image.height);
    if(invertY){
        for(int i=0; i<image.height; i++){
            row_pointers[image.height-i-1] = image.data.data() + i*image.pitchBytes;
        }
    }else{
        for(int i=0; i<image.height; i++){
            row_pointers[i] = image.data.data() + i*image.pitchBytes;
        }
    }

    /* and now we just write the whole image; libpng takes care of interlacing
     * for us */
    png_write_image(png_ptr, row_pointers.data());


    /* since that's it, we also close out the end of the PNG file now--if we
     * had any text or time info to write after the IDATs, second argument
     * would be info_ptr, but we optimize slightly by sending NULL pointer: */
    png_write_end(png_ptr, NULL);

    return 0;
}

bool writePNG(PngImage& img, const std::string &path, bool invertY)
{

    FILE *fp = fopen(path.c_str(), "wb");
    if (!fp)
    {
        std::cout << "could not open file: " << path.c_str() << std::endl;
        return false;
    }

    PNGHelperData data;
    data.outfile = fp;


    if(writepng_init(img,&data)!=0){
        std::cout<<"error write png init"<<std::endl;
    }
    if(writepng_encode_image(img,&data,invertY)!=0){
        std::cout<<"error write encode image"<<std::endl;
    }


    png_structp png_ptr = (png_structp)data.png_ptr;
    png_infop info_ptr = (png_infop)data.info_ptr;

    if (png_ptr && info_ptr)
        png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(fp);

    std::cout << "Write PNG success! " << path << " Size: " << img.width << "x" << img.height << " Type: " << img.channels << "x" << img.bit_depth << " " << colorTypeToString(img.color_type) << std::endl;

    return true;

}

}
