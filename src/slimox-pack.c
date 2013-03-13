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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "slimox-ppm.h"
#include "slimox-buf.h"

#define FILENAME_MAXLEN 32768

#if defined(WIN32) || defined(WIN64)
#include <windows.h>

#define tmpfile()     tmpfile_mingw()
#define mkdir(_1, _2) mkdir(_1)
#define lstat(_1, _2) stat(_1, _2)

static inline FILE* tmpfile_mingw() {
    static char filename[MAX_PATH] = {0};

    GetTempFileName(".", NULL, 0, filename);
    return fopen(filename,"w+bTD");
}

static inline const char* __path(const char* path) {
    static char newpath[FILENAME_MAXLEN + 1];
    char* p;

    strcpy(newpath, path);
    for(p = newpath; *p != '\0'; p++) {
        if(*p == '\x2f') {
            *p = '\x5c';
        }
    }
    if(*--p == '\\') {
        *p = 0;
    }
    return newpath;
}

#else /* non-windows */
#define __path(_1) _1
#endif



extern int slimox_encode(buf_t* ib, buf_t* ob, ppm_model_t* ppm, FILE* fout_sync);
extern int slimox_decode(buf_t* ib, buf_t* ob, ppm_model_t* ppm);

static int traverse_directory_make_header(const char* path, const char* root, FILE* outstream) {
    char fullpath[FILENAME_MAXLEN + 1];
    char rootpath[FILENAME_MAXLEN + 1];
    DIR* dir;
    struct dirent* item;
    struct stat st;

    /* write file information of root dir */
    if(*root) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, root);
    } else {
        snprintf(fullpath, sizeof(fullpath), "%s", path);
    }
    if(lstat(__path(fullpath), &st) == -1) {
        fprintf(stderr, "warning: ignore file '%s': stat failed.\n", __path(fullpath));
        return -1;
    }
    fprintf(outstream, "%s//%lx.%lu\n", root, (unsigned long)st.st_mode, 0l);

    /* open directory */
    if((dir = opendir(fullpath)) == NULL) {
        fprintf(stderr, "warning: open directory '%s' failed.\n", fullpath);
        return -1;
    }

    /* traverse directory */
    while((item = readdir(dir)) != NULL) {
        if(strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0) {
            continue;
        }

        /* construct fullpath */
        if(strlen(path) + 1 + strlen(root) + 1 + strlen(item->d_name) > FILENAME_MAXLEN) {
            fprintf(stderr, "warning: ignore file '%s/%s': filename too long.\n", path, item->d_name);
            continue;
        }
        snprintf(fullpath, sizeof(fullpath), "%s/%s/%s", path, root, item->d_name);
        snprintf(rootpath, sizeof(rootpath), "%s/%s", root, item->d_name);

        /* stat */
        if(lstat(__path(fullpath), &st) == -1) {
            fprintf(stderr, "warning: ignore file '%s/%s': stat failed.\n", path, item->d_name);
            continue;
        }
        if(!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode)) {
            fprintf(stderr, "warning: ignore file '%s/%s': not a regular file.\n", path, item->d_name);
            continue;
        }

        /* write file item information */
        if(S_ISREG(st.st_mode)) {
            fprintf(outstream, "%s//%lx.%lu\n", rootpath, (unsigned long)st.st_mode, (unsigned long)st.st_size);
        } else {
            traverse_directory_make_header(path, rootpath, outstream);
        }
    }
    closedir(dir);
    return 0;
}

int pack(const char* path, const char* outfile, ppm_model_t* ppm, int block_size) {
    FILE* tmpstream;
    FILE* instream;
    FILE* outstream;
    int c;
    char filename[FILENAME_MAXLEN + 1];
    char fullpath[FILENAME_MAXLEN + 1];
    int len;
    unsigned long mode;
    unsigned long size;
    unsigned long pos_sync;
    unsigned long in_size;
    unsigned long out_size;
    buf_t* ib;
    buf_t* ob;

    if((outstream = fopen(outfile, "wb")) == NULL) {
        fprintf(stderr, "error: open '%s' failed.\n", outfile);
        return -1;
    }

    /* write header into temporary file */
    if((tmpstream = tmpfile()) == NULL) {
        fprintf(stderr, "error: cannot create temporary file.\n");
        fclose(outstream);
        return -1;
    }
    if(traverse_directory_make_header(path, "", tmpstream) == -1) {
        fprintf(stderr, "error: open root directory '%s' failed.\n", path);
        fclose(outstream);
        return -1;
    }
    putc(0, tmpstream); /* End-of-Header */
    rewind(tmpstream);

    /* dump header to outstream */
    while((c = getc(tmpstream)) != EOF) {
        putc(c, outstream);
    }
    fflush(outstream);
    rewind(tmpstream);

    ib = buf_new();
    ob = buf_new();
    buf_resize(ib, block_size);

    /* scan file list and compress data */
    while(ungetc(getc(tmpstream), tmpstream) != 0) {
        len = 0;
        while(len < 2 || (filename[len - 1] != '/' || filename[len - 2] != '/')) {
            filename[len++] = getc(tmpstream);
        }
        filename[--len] = 0;
        filename[--len] = 0;
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);
        fscanf(tmpstream, "%lx.%lu\n", &mode, &size);

        /* compress a regular file */
        if(S_ISREG(mode)) {
            if((instream = fopen(fullpath, "rb")) == NULL) {
                fprintf(stderr, "error: open file '%s' failed.\n", fullpath);
                break;
            }

            fprintf(stderr, "compressing %s...\n", fullpath);
            in_size = 0;
            while((ib->m_size = fread(ib->m_data, 1, ib->m_capacity, instream)) > 0) {
                pos_sync = ftell(outstream);
                buf_resize(ob, 0);
                fwrite(&ob->m_size, sizeof(ob->m_size), 1, outstream);

                /* compress */
                in_size += ib->m_size;
                out_size = slimox_encode(ib, ob, ppm, outstream);

                /* write back out_size */
                fseek(outstream, pos_sync, SEEK_SET);
                fwrite(&out_size, sizeof(ob->m_size), 1, outstream);
                fseek(outstream, 0, SEEK_END);
            }
            fclose(instream);

            if(in_size != size) { /* file size not match, error */
                fprintf(stderr, "error: '%s' file size changed while compressing.\n", fullpath);
                break;
            }
        }
    }
    fclose(tmpstream);
    fclose(outstream);
    buf_del(ib);
    buf_del(ob);
    return 0;
}


int unpack(const char* infile, const char* path, ppm_model_t* ppm, int block_size) {
    FILE* tmpstream;
    FILE* instream;
    FILE* outstream;
    int c;
    char filename[FILENAME_MAXLEN + 1];
    char fullpath[FILENAME_MAXLEN + 1];
    int len;
    unsigned long mode;
    unsigned long size;
    unsigned long out_size;
    struct stat st;
    buf_t* ib;
    buf_t* ob;

    /* open input file */
    if((instream = fopen(infile, "rb")) == NULL) {
        fprintf(stderr, "error: open '%s' failed.\n", infile);
        return -1;
    }

    /* dump header to tmpstream */
    if((tmpstream = tmpfile()) == NULL) {
        fprintf(stderr, "error: cannot create temporary file.\n");
        fclose(instream);
        return -1;
    }
    while((c = getc(instream)) != 0) {
        putc(c, tmpstream);
    }
    putc(0, tmpstream);
    rewind(tmpstream);

    ib = buf_new();
    ob = buf_new();
    buf_resize(ib, block_size);

    /* scan file list and compress data */
    while(ungetc(getc(tmpstream), tmpstream) != 0) {
        len = 0;
        while(len < 2 || (filename[len - 1] != '/' || filename[len - 2] != '/')) {
            filename[len++] = getc(tmpstream);
        }
        filename[--len] = 0;
        filename[--len] = 0;
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);
        fscanf(tmpstream, "%lx.%lu\n", &mode, &size);

        /* create directory */
        if(S_ISDIR(mode)) {
            mkdir(__path(fullpath), mode);
            if(lstat(__path(fullpath), &st) == -1 || !S_ISDIR(st.st_mode)) {
                fprintf(stderr, "error: cannot create directory '%s'.\n", path);
                break;
            }
        }

        /* compress a regular file */
        if(S_ISREG(mode)) {
            if((outstream = fopen(fullpath, "wb")) == NULL) {
                fprintf(stderr, "error: open '%s' failed.\n", fullpath);
                break;
            }
            fprintf(stderr, "decompressing %s...\n", fullpath);
            out_size = 0;
            while(out_size < size && fread(&ib->m_size, sizeof(ib->m_size), 1, instream) > 0) {
                buf_resize(ib, ib->m_size);
                buf_resize(ob, 0);
                fread(ib->m_data, 1, ib->m_size, instream);

                slimox_decode(ib, ob, ppm);
                fwrite(ob->m_data, 1, ob->m_size, outstream);
                out_size += ob->m_size;
            }
            fclose(outstream);
        }
    }
    fclose(tmpstream);
    fclose(instream);
    buf_del(ib);
    buf_del(ob);
    return 0;
}
