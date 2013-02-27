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
#include <time.h>
#include "slimox-ppm.h"
#include "slimox-buf.h"

#if defined(_WIN32) || defined(_WIN64) /* windows ports */
#include <fcntl.h> /* for setmode() */
#endif

extern int slimox_encode(buf_t* ib, buf_t* ob, ppm_model_t* ppm, FILE* fout_sync);
extern int slimox_decode(buf_t* ib, buf_t* ob, ppm_model_t* ppm);

const char* msg_start = (
        "============================================\n"
        " comprox: an LZP-PPM compressor             \n"
        " written by Zhang Li <richselian@gmail.com> \n"
        "============================================\n");
const char* msg_help = (
        "to compress:   slimox e [input] [output]\n"
        "to decompress: slimox d [input] [output]\n"
        "\n"
        "optional SWITCH:\n"
        " -q  quiet mode.\n"
        " -b  set block size(MB), default = 16.\n");

int main(int argc, char** argv) {
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
    clock_t timer_start = clock();

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
    if(argc == 4 && strcmp(argv[1], "e") == 0) {
        fprintf(stderr, "compressing %s to %s...\n", argv[2], argv[3]);
        fi = fopen(argv[2], "rb");
        fo = fopen(argv[3], "wb");
        if(fi && fo) {
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
            fprintf(stderr, "%d => %d in %.2lfs\n", src_size, dst_size, (double)(clock() - timer_start) / CLOCKS_PER_SEC);
        } else {
            perror("error: fopen()");
            dst_size = -1;
        }
        if(fi) fclose(fi);
        if(fo) fclose(fo);
    }
    if(argc == 4 && strcmp(argv[1], "d") == 0) {
        fprintf(stderr, "decompressing %s to %s...\n", argv[2], argv[3]);
        fi = fopen(argv[2], "rb");
        fo = fopen(argv[3], "wb");
        if(fi && fo) {
            while(fread(&ib->m_size, sizeof(ib->m_size), 1, fi) > 0) {
                buf_resize(ib, ib->m_size);
                buf_resize(ob, 0);
                fread(ib->m_data, 1, ib->m_size, fi);

                slimox_decode(ib, ob, ppm);
                fwrite(ob->m_data, 1, ob->m_size, fo);
            }
            src_size = ftell(fi);
            dst_size = ftell(fo);
            fprintf(stderr, "%d <= %d in %.2lfs\n", dst_size, src_size, (double)(clock() - timer_start) / CLOCKS_PER_SEC);
        } else {
            perror("error: fopen()");
            dst_size = -1;
        }
        if(fi) fclose(fi);
        if(fo) fclose(fo);
    }

    if(dst_size == 0) { /* error happend? */
        fprintf(stderr, msg_help);
    }
    ppm_model_del(ppm);
    buf_del(ib);
    buf_del(ob);
    return 0;
}
