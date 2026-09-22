#include <stdio.h>
#include<string.h>
#include "encode.h"
#include"common.h"
#include "types.h"

/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    rewind(fptr_image);

    // Return image capacity
    return width * height * 3;
}

/* 
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files(EncodeInfo *encInfo)
{
    // Src Image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "rb");
    // Do Error handling
    if (encInfo->fptr_src_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);

    	return e_failure;
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    // Do Error handling
    if (encInfo->fptr_secret == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);

    	return e_failure;
    }

    // Stego Image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "wb");
    // Do Error handling
    if (encInfo->fptr_stego_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);

    	return e_failure;
    }

    // No failure return e_success
    return e_success;
}


Status read_and_validate_encode_args(char *argv[], int argc, EncodeInfo *encInfo)
{
    // argv[2] -> .bmp
    //     -> yes -> store it in structure
    //     -> error - return e_failure;
    if(!(strstr(argv[2], ".bmp")))
    {
        fprintf(stderr, "Error: (.bmp) is missing in the source file name\n");
        return e_failure;
    }
    encInfo->src_image_fname = argv[2];

    //  argv[3] -> .txt
    //     -> yes -> store it in structure and store the extn in structure.
    //     -> error - return e_failure;
    if(!(strstr(argv[3], ".txt")))
    {
        fprintf(stderr, "Error: (.txt) is missing in the secret file name\n");
        return e_failure;
    }
    encInfo->secret_fname = argv[3];
    encInfo->extn_secret_file = strstr(argv[3], ".");

    // optional destination image
    // argv[4] -> passed or not
    //     -> passed -> .bmp
    //         -> yes -> store it in structure
    //         -> (extn)error - return e_failure;
    //     -> not passed -> store the "stego.bmp" into the structure.
    if(argc >= 5)
    {
        if(!(strstr(argv[4], ".bmp")))
        {
            fprintf(stderr, "Error: (.bmp) is missing in the destination file name\n");
            return e_failure;
        }
        encInfo->stego_image_fname = argv[4];
    }
    else
    {
        encInfo->stego_image_fname = "stego.bmp";
        // encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "wb");
    }
    return e_success;
}

Status do_encoding(EncodeInfo *encInfo)
{
    if(open_files(encInfo) == e_failure)
    {
        fprintf(stderr, "Error: File not found\n");
        return e_failure;
    } 
    
    // ==> call function 
    // ==> check_capacity();
    if(check_capacity(encInfo) == e_failure)
    {
        fprintf(stderr, "Error: Image doesn't have enough capacity\n");
        return e_failure;
    }

    if(copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        fprintf(stderr, "Error: Failed to copy BMP header\n");
        return e_failure;
    }

    if(encode_magic_string(MAGIC_STRING, encInfo) == e_failure)
    {
        fprintf(stderr,"Error: Failed to encode magic string\n");
        return e_failure;
    }

    if (encode_secret_file_extn_size(strlen(encInfo->extn_secret_file), encInfo) == e_failure)
    {
        fprintf(stderr, "Error: Failed to encode extension size\n");
        return e_failure;
    }

    if (encode_secret_file_extn(encInfo->extn_secret_file, encInfo) == e_failure)
    {
        fprintf(stderr, "Error: Failed to encode extension\n");
        return e_failure;
    }

    if (encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_failure)
    {
        fprintf(stderr, "Error: Failed to encode secret file size\n");
        return e_failure;
    }

    if (encode_secret_file_data(encInfo) == e_failure)
    {
        fprintf(stderr, "Error: Failed to encode secret file data\n");
        return e_failure;
    }

    if (copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        fprintf(stderr, "Error: Failed to copy remaining image data\n");
        return e_failure;
    }
    
    return e_success;
}

Status check_capacity(EncodeInfo *encInfo)
{
    //     => call get_image_size_for_bmp
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);

    //     => call get file size
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);

    // stego.bmp > secret.txt * 8
    if(!(encInfo->image_capacity > (2 + 4 + 4 + 4 + (encInfo->size_secret_file))*8))
    {
        return e_failure;
    }
    return e_success;
}

uint get_file_size(FILE *fptr)
{
    fseek(fptr, 0, SEEK_END);
    uint size = ftell(fptr);
    rewind(fptr);

    return size;
}

Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    char header[54];                                   // the "bucket" - 54 bytes of memory
    rewind(fptr_src_image);                            //  start reading from byte 0
    fread(header, sizeof(char), 54, fptr_src_image);   // scoop 54 bytes FROM source INTO header
    fwrite(header, sizeof(char), 54, fptr_dest_image); // pour those 54 bytes FROM header INTO dest

    return e_success;
}

Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    int len = strlen(magic_string);
    for(int i = 0; i < len; i++)
    {
        char image_buffer[8];
        magic_string[i];
        fread(image_buffer, sizeof(char), 8, encInfo->fptr_src_image);
        encode_byte_to_lsb(magic_string[i], image_buffer);
        fwrite(image_buffer, sizeof(char), 8, encInfo->fptr_stego_image);
    }
    return e_success;
}

Status encode_secret_file_extn_size(int size, EncodeInfo *encInfo)
{
    char image_buffer[32];
    fread(image_buffer, sizeof(char), 32, encInfo->fptr_src_image);
    encode_size_to_lsb(size, image_buffer);
    fwrite(image_buffer, sizeof(char), 32, encInfo->fptr_stego_image);
    
    return e_success;
}

Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    int len = strlen(file_extn);
    for(int i = 0; i < len; i++)
    {
        char image_buffer[8];
        file_extn[i];
        fread(image_buffer, sizeof(char), 8, encInfo->fptr_src_image);
        encode_byte_to_lsb(file_extn[i], image_buffer);
        fwrite(image_buffer, sizeof(char), 8, encInfo->fptr_stego_image);
    }
    return e_success;
}

Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    char image_buffer[32];
    fread(image_buffer, sizeof(char), 32, encInfo->fptr_src_image);
    encode_size_to_lsb(file_size, image_buffer);
    fwrite(image_buffer, sizeof(char), 32, encInfo->fptr_stego_image);
    
    return e_success;
}

Status encode_secret_file_data(EncodeInfo *encInfo)
{
    for (int i = 0; i < encInfo->size_secret_file; i++)
    {
        char ch = fgetc(encInfo->fptr_secret); 
        char image_buffer[8];
        fread(image_buffer, sizeof(char), 8, encInfo->fptr_src_image);
        encode_byte_to_lsb(ch, image_buffer);
        fwrite(image_buffer, sizeof(char), 8, encInfo->fptr_stego_image);
    }
    return e_success;
}

Status encode_data_to_image(char *data, int size, FILE *fptr_src_image, FILE *fptr_stego_image)
{
    char image_buffer[8];
    for (int i = 0; i < size; i++)
    {
        fread(image_buffer, sizeof(char), 8, fptr_src_image);
        encode_byte_to_lsb(data[i], image_buffer);
        fwrite(image_buffer, sizeof(char), 8, fptr_stego_image);
    }
    return e_success;
}

Status encode_byte_to_lsb(char data, char *image_buffer)
{
    int i = 0;
    while(i < 8)
    {
        int bit = (data >> i) & 1;
        image_buffer[i] = image_buffer[i] & 0xFE;
        image_buffer[i] = image_buffer[i] | bit;
        i++;
    }
    return e_success;
}

Status encode_size_to_lsb(int size, char *imageBuffer)
{
    int i = 0;
    while(i < (sizeof(int)*8))
    {
        int bit = (size >> i) & 1;
        imageBuffer[i] = imageBuffer[i] & 0xFE;
        imageBuffer[i] = imageBuffer[i] | bit;
        i++;
    }
    return e_success;
}

Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    char ch;
    while (fread(&ch, sizeof(char), 1, fptr_src) == 1)
    {
        fwrite(&ch, sizeof(char), 1, fptr_dest);
    }
    return e_success;
}