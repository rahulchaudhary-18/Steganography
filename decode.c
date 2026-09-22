#include <stdio.h>
#include<string.h>
#include "encode.h"
#include"decode.h"
#include"common.h"
#include "types.h"

char decode_byte_from_lsb(char *image_buffer)
{
    char result = 0;
    for (int i = 0; i < 8; i++)
    {
        int bit = image_buffer[i] & 1;
        result = result | (bit << i);
    }
    return result;
}

Status open_files_decode(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "rb");
    if(decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr,"Error: Unable to open file %s\n", decInfo->stego_image_fname);
        return e_failure;
    }

    // Secret file (output, write text)
    decInfo->fptr_secret = fopen(decInfo->secret_fname, "w");
    if (decInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", decInfo->secret_fname);
        return e_failure;
    }

    return e_success;
}

Status decode_magic_string(DecodeInfo *decInfo)
{
    fseek(decInfo->fptr_stego_image, 54, SEEK_SET);   // skip the 54-byte BMP header

    int len = strlen(MAGIC_STRING);
    char decode_magic[3];     // 2 characters + null terminator
    for(int i = 0; i < len; i++)
    {
        char image_buffer[8];
        fread(image_buffer, sizeof(char), 8, decInfo->fptr_stego_image);
        decode_magic[i] = decode_byte_from_lsb(image_buffer);
    }
    decode_magic[len] = '\0';    // null-terminate so strcmp works correctly
    if((strcmp(decode_magic, MAGIC_STRING)) != 0)
    {
        fprintf(stderr, "Error: This doesn't appear to be a valid stego image\n");
        return e_failure;
    }

    return e_success;
}

int decode_size_from_lsb(char *image_buffer)
{
    int result = 0;
    for (int i = 0; i <  (sizeof(int)*8); i++)
    {
        int bit = image_buffer[i] & 1;
        result = result | (bit << i);
    }
    return result;
}

int decode_secret_file_extn_size(DecodeInfo *decInfo)
{
    char image_buffer[32];
    fread(image_buffer, sizeof(char), 32, decInfo->fptr_stego_image);
    int size = decode_size_from_lsb(image_buffer);
    return size;
}

Status decode_secret_file_extn(int extn_size, DecodeInfo *decInfo)
{
    char decoded_extn[10];   // room for the extension + null terminator

    for (int i = 0; i < extn_size; i++)
    {
        char image_buffer[8];
        fread(image_buffer, sizeof(char), 8, decInfo->fptr_stego_image);
        decoded_extn[i] = decode_byte_from_lsb(image_buffer);
    }
    decoded_extn[extn_size] = '\0';

    return e_success;
}

long decode_secret_file_size(DecodeInfo *decInfo)
{
    char image_buffer[32];
    fread(image_buffer, sizeof(char), 32, decInfo->fptr_stego_image);
    long size = decode_size_from_lsb(image_buffer);
    return size;
}

Status decode_secret_file_data(DecodeInfo *decInfo)
{
    for (int i = 0; i < decInfo->size_secret_file; i++)
    {
        char image_buffer[8];
        fread(image_buffer, sizeof(char), 8, decInfo->fptr_stego_image);
        char ch = decode_byte_from_lsb(image_buffer);
        fputc(ch, decInfo->fptr_secret);
    }

    return e_success;
}

Status do_decoding(DecodeInfo *decInfo)
{
    // 1. open_files_decode
    // 2. decode_magic_string — check it matches "#*", fail if not
    // 3. decode_secret_file_extn_size — get the extension length
    // 4. decode_secret_file_extn — get the extension itself
    // 5. decode_secret_file_size — get the secret data's length
    // 6. decode_secret_file_data — extract the actual message, write it to a new file

    if (open_files_decode(decInfo) == e_failure)
    {
        fprintf(stderr, "Error: File not found\n");
        return e_failure;
    }

    if (decode_magic_string(decInfo) == e_failure)
    {
        fprintf(stderr, "Error: Failed to decode magic string\n");
        return e_failure;
    }

    int extn_size = decode_secret_file_extn_size(decInfo);

    if (decode_secret_file_extn(extn_size, decInfo) == e_failure)
    {
        fprintf(stderr, "Error: Failed to decode extension\n");
        return e_failure;
    }

    decInfo->size_secret_file = decode_secret_file_size(decInfo);

    if (decode_secret_file_data(decInfo) == e_failure)
    {
        fprintf(stderr, "Error: Failed to decode secret file data\n");
        return e_failure;
    }

    return e_success;
}


Status read_and_validate_decode_args(char *argv[], int argc, DecodeInfo *decInfo)
{
    if (!(strstr(argv[2], ".bmp")))
    {
        fprintf(stderr, "Error: (.bmp) is missing in the source file name\n");
        return e_failure;
    }
    decInfo->stego_image_fname = argv[2];

    if (argc >= 4)
    {
        decInfo->secret_fname = argv[3];
    }
    else
    {
        decInfo->secret_fname = "decoded.txt";
    }

    return e_success;
}