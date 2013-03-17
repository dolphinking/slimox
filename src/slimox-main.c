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
        "to pack directory:   slimox [SWITCH] epack [input-dir] [output]\n"
        "to unpack directory: slimox [SWITCH] dpack [input] [output-dir]\n"
        "to compress file:    slimox [SWITCH] e [input] [output]\n"
        "to decompress file:  slimox [SWITCH] d [input] [output]\n"
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

    FILE* fi;
    FILE* fo;
    char option;
    uint32_t block_size = 16777216; /* 16MB by default */
    uint32_t out_size;
    uint32_t src_size = 0;
    uint32_t dst_size = 0;
    uint32_t pos_sync;
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
            memcpy(&argv[1], &argv[2], (--argc) * sizeof(*argv));
            continue;
        }
        if(option == 'b') { /* set blocksize */
            if(sscanf(argv[1] + 2, "%d", &block_size) > 0) {
                block_size *= 1048576;
            } else {
                fprintf(stderr, "error: Invalid blocksize\n");
                exit(-1);
            }
            memcpy(&argv[1], &argv[2], (--argc) * sizeof(*argv));
            continue;
        }
        fprintf(stderr, "error: Invalid argument\n");
        exit(-1);
    }
    buf_resize(ib, block_size);

    /* start! */
    fprintf(stderr, msg_start);
    if(argc == 4 && strcmp(argv[1], "epack") == 0) { /* pack directory */
        operation = OPERATION_PACK;
        pack(argv[2], argv[3], ppm, block_size);
    }
    if(argc == 4 && strcmp(argv[1], "dpack") == 0) { /* unpack directory */
        operation = OPERATION_PACK;
        unpack(argv[2], argv[3], ppm, block_size);
    }
    if(argc == 4 && strcmp(argv[1], "e") == 0) { /* compress single file */
        operation = OPERATION_COMP;
        if((fi = fopen(argv[2], "rb")) == NULL) {
            fprintf(stderr, "open '%s' failed.\n", argv[2]);
            goto SlimoxMain_final;
        }
        if((fo = fopen(argv[3], "wb")) == NULL) {
            fprintf(stderr, "open '%s' failed.\n", argv[3]);
            fclose(fi);
            goto SlimoxMain_final;
        }
        fprintf(stderr, "compressing %s to %s...\n", argv[2], argv[3]);

        while((ib->m_size = fread(ib->m_data, 1, ib->m_capacity, fi)) > 0) {
            pos_sync = ftell(fo);
            buf_resize(ob, 0);
            fwrite(&ob->m_size, sizeof(ob->m_size), 1, fo);

            out_size = slimox_encode(ib, ob, ppm, fo);
            fseek(fo, pos_sync, SEEK_SET);
            fwrite(&out_size, sizeof(ob->m_size), 1, fo);
            fseek(fo, 0, SEEK_END);
        }
        src_size = ftell(fi);
        dst_size = ftell(fo);
        fclose(fi);
        fclose(fo);

        /* timer end */
        gettimeofday(&time_end, NULL);
        cost_time = (time_end.tv_sec - time_start.tv_sec) + (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
        fprintf(stderr, "%d => %d in %.2lfs\n", src_size, dst_size, cost_time);
    }
    if(argc == 4 && strcmp(argv[1], "d") == 0) { /* decompress single file */
        operation = OPERATION_DECOMP;
        if((fi = fopen(argv[2], "rb")) == NULL) {
            fprintf(stderr, "open '%s' failed.\n", argv[2]);
            goto SlimoxMain_final;
        }
        if((fo = fopen(argv[3], "wb")) == NULL) {
            fprintf(stderr, "open '%s' failed.\n", argv[3]);
            fclose(fi);
            goto SlimoxMain_final;
        }
        fprintf(stderr, "decompressing %s to %s...\n", argv[2], argv[3]);

        while(fread(&ib->m_size, sizeof(ib->m_size), 1, fi) > 0) {
            buf_resize(ib, ib->m_size);
            buf_resize(ob, 0);
            fread(ib->m_data, 1, ib->m_size, fi);

            slimox_decode(ib, ob, ppm);
            fwrite(ob->m_data, 1, ob->m_size, fo);
        }
        src_size = ftell(fi);
        dst_size = ftell(fo);
        fclose(fi);
        fclose(fo);

        /* timer end */
        gettimeofday(&time_end, NULL);
        cost_time = (time_end.tv_sec - time_start.tv_sec) + (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
        fprintf(stderr, "%d <= %d in %.2lfs\n", dst_size, src_size, cost_time);
    }

SlimoxMain_final:
    if(operation == OPERATION_INVALID) { /* bad operation */
        fprintf(stderr, msg_help);
    }
    ppm_model_del(ppm);
    buf_del(ib);
    buf_del(ob);
    return 0;
}
