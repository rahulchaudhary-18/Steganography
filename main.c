#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "decode.h"
#include "types.h"

OperationType check_operation_type(char *argv[]);
// {
//     // argv[1] -> -e -> e_encode;
//     //         -> -d -> e_decode;
//     //         -> e_unsupported;
// }

int main(int argc, char *argv[])
{
    // data_type variable = function call();
    OperationType res = check_operation_type(argv);
    if(res == e_encode)
    {
        // create a structure variable
        EncodeInfo encInfo;
        if(read_and_validate_encode_args(argv, argc, &encInfo) == e_success)
        {
            //do encoding
            do_encoding(&encInfo);
        }
        else
        {
            //error -> exit.
            fprintf(stderr, "Error: Invalid argument for encoding\n");
            return e_failure;
        }
    }
    else if(res == e_decode)
    {
        DecodeInfo decInfo;
        if(read_and_validate_decode_args(argv, argc, &decInfo) == e_success)
        {
            do_decoding(&decInfo);
        }
        else
        {
            fprintf(stderr, "Error: Invalid argument for decoding\n");
            return e_failure;
        }
    }
    else
    {
        // -> e_unsupported -> error -> exit
        fprintf(stderr, "Error: unsupported operation type\n");
        return e_failure;
    }
    return e_success;
}

OperationType check_operation_type(char *argv[])
{
    if(!(strcmp(argv[1], "-e")))
    {
        return e_encode;
    }
    else if(!(strcmp(argv[1], "-d")))
    {
        return e_decode;
    }
    else
    {
        return e_unsupported;
    }
}
