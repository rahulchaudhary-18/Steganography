#ifndef DECODE_H
#define DECODE_H

#include "types.h"

/*
 * Structure to store information required for
 * decoding secret file from a stego Image
 */
typedef struct _DecodeInfo
{
    /* Stego Image info */
    char *stego_image_fname;
    FILE *fptr_stego_image;

    /* Secret File Info */
    char *secret_fname;
    FILE *fptr_secret;
    char extn_secret_file[5];
    long size_secret_file;

} DecodeInfo;

/* Decoding function prototypes */

/* Read and validate Decode args from argv */
Status read_and_validate_decode_args(char *argv[], int argc, DecodeInfo *decInfo);

/* Perform the decoding */
Status do_decoding(DecodeInfo *decInfo);

/* Get File pointers for i/p and o/p files */
Status open_files_decode(DecodeInfo *decInfo);

/* Decode Magic String */
Status decode_magic_string(DecodeInfo *decInfo);

/* Decode secret file extension size */
int decode_secret_file_extn_size(DecodeInfo *decInfo);

/* Decode secret file extension */
Status decode_secret_file_extn(int extn_size, DecodeInfo *decInfo);

/* Decode secret file size */
long decode_secret_file_size(DecodeInfo *decInfo);

/* Decode secret file data */
Status decode_secret_file_data(DecodeInfo *decInfo);

/* Decode a byte from LSB of image data array */
char decode_byte_from_lsb(char *image_buffer);

/* Decode a size (int) from LSB of image data array */
int decode_size_from_lsb(char *image_buffer);

#endif