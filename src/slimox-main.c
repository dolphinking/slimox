/*
 * Copyright (C) 2012-2013 by Zhang Li <RichSelian at gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "slimox-ppm.h"
#include "slimox-buf.h"

#if defined(_WIN32) || defined(_WIN64) /* windows ports */
#include <fcntl.h> /* for setmode() */
#endif

extern int slimox_encode(buf_t* ib, buf_t* ob, ppm_model_t* ppm, FILE* fout_sync);
extern int slimox_decode(buf_t* ib, buf_t* ob, ppm_model_t* ppm);

extern int pack(const char* path, const char* outfile, ppm_model_t* ppm, int block_size);
extern int unpack(const char* infile, const char* path, ppm_model_t* ppm, int block_size);

const char* msg_start = (
        "============================================\n"
        " comprox: an LZP-PPM compressor/archiver    \n"
        " written by Zhang Li <richselian@gmail.com> \n"
        "============================================\n");
const char* msg_help = (
        "to encode: slimox [SWITCH] e [input (file/dir)] [output]\n"
        "to decode: slimox [SWITCH] d [input] [output (file/dir)]\n"
        "\n"
        "optional SWITCH:\n"
        " -q  quiet mode.\n"
        " -b  set block size(MB), default = 16.\n");

int main(int argc, char** argv) {
    enum {
        OPERATION_INVALID,
        OPERATION_COMP,
        OPERATION_DECOMP,
        OPERATION_PACK,
        OPERATION_UNPACK,
    } operation = OPERATION_INVALID;

    char option;
    uint32_t block_size = 16777216; /* 16MB by default */
    ppm_model_t* ppm = ppm_model_new();
    buf_t* ib = buf_new();
    buf_t* ob = buf_new();
    struct timeval time_start;
    struct timeval time_end;
    double cost_time;

    /* timer start */
    gettimeofday(&time_start, NULL);

#if defined(_WIN32) || defined(_WIN64)
    /* we need to set stdin/stdout to binary mode under windows */
    setmode(fileno(stdin), O_BINARY);
    setmode(fileno(stdout), O_BINARY);
#endif

    /* process args */
    while(argc > 1 && sscanf(argv[1], "-%c", &option) > 0) {
        if(option == 'q') { /* quiet mode */
            fclose(stderr);
            memmove(&argv[1], &argv[2], (--argc) * sizeof(*argv));
            continue;
        }
        if(option == 'b') { /* set blocksize */
            if(sscanf(argv[1] + 2, "%d", &block_size) > 0) {
                block_size *= 1048576;
            } else {
                fprintf(stderr, "error: Invalid blocksize\n");
                exit(-1);
            }
            memmove(&argv[1], &argv[2], (--argc) * sizeof(*argv));
            continue;
        }
        fprintf(stderr, "error: Invalid argument\n");
        exit(-1);
    }
    buf_resize(ib, block_size);

    /* start! */
    fprintf(stderr, msg_start);
    if(argc == 4 && strcmp(argv[1], "e") == 0) { /* pack directory */
        operation = OPERATION_PACK;
        if(pack(argv[2], argv[3], ppm, block_size) == -1) {
            abort();
        }

        /* timer end */
        gettimeofday(&time_end, NULL);
        cost_time = (time_end.tv_sec - time_start.tv_sec) + (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
        fprintf(stderr, "encode done in %.2lfsec\n", cost_time);
    }
    if(argc == 4 && strcmp(argv[1], "d") == 0) { /* unpack directory */
        operation = OPERATION_PACK;
        if(unpack(argv[2], argv[3], ppm, block_size) == -1) {
            abort();
        }

        /* timer end */
        gettimeofday(&time_end, NULL);
        cost_time = (time_end.tv_sec - time_start.tv_sec) + (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
        fprintf(stderr, "encode done in %.2lfsec\n", cost_time);
    }

    if(operation == OPERATION_INVALID) { /* bad operation */
        fprintf(stderr, msg_help);
    }
    ppm_model_del(ppm);
    buf_del(ib);
    buf_del(ob);
    return 0;
}
