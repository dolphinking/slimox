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
#include <assert.h>

#include "slimox-ppm.h"
#include "slimox-buf.h"
#include "utils/string-utils.h"

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
#define PATH_SEP    "\x5c"
#define PATH_JOINER "%s\x5c%s"

#else /* non-windows */
#define PATH_SEP    "\x2f"
#define PATH_JOINER "%s\x2f%s"
#endif

extern int slimox_encode(buf_t* ib, buf_t* ob, ppm_model_t* ppm, FILE* fout_sync);
extern int slimox_decode(buf_t* ib, buf_t* ob, ppm_model_t* ppm);

/* represent a file item (regular file or directory) */
struct file_item {
    uint32_t  m_mode;
    uint32_t  m_size;
    string_t* m_path_to_root;
};

/* operations to a file item struct */
static inline int file_item_from_path(struct file_item* item, const string_t* full_path, const string_t* root_path) {
    struct stat statbuf;

    if(stat(full_path->str, &statbuf) != -1 && (S_ISREG(statbuf.st_mode) || S_ISDIR(statbuf.st_mode))) {
        if(S_ISDIR(statbuf.st_mode)) {
            statbuf.st_size = 0;
        }
        item->m_mode = statbuf.st_mode;
        item->m_size = statbuf.st_size;
        str_assign(item->m_path_to_root, root_path->str);
        return 1;
    }
    return 0;
}
static inline struct file_item* file_item_new() {
    struct file_item* item = malloc(sizeof(struct file_item));
    item->m_mode = 0;
    item->m_size = 0;
    item->m_path_to_root = str_new();
    return item;
}
static inline void file_item_del(struct file_item* item) {
    str_del(item->m_path_to_root);
    free(item);
    return;
}

static inline void file_item_put(struct file_item* item, FILE* fp) {
    size_t i;

    fwrite(&item->m_mode, sizeof(item->m_mode), 1, fp);
    fwrite(&item->m_size, sizeof(item->m_size), 1, fp);
    fwrite(&item->m_path_to_root->len, sizeof(item->m_path_to_root->len), 1, fp);

    for(i = 0; i < item->m_path_to_root->len; i++) {
        if(item->m_path_to_root->str[i] == PATH_SEP[0]) { /* replace path seperator with 0 */
            fputc(0, fp);
        } else {
            fputc(item->m_path_to_root->str[i], fp);
        }
    }
    return;
}
static inline void file_item_get(struct file_item* item, FILE* fp) {
    size_t len;
    size_t i;
    int c;

    fread(&item->m_mode, sizeof(item->m_mode), 1, fp);
    fread(&item->m_size, sizeof(item->m_size), 1, fp);
    fread(&len, sizeof(item->m_path_to_root->len), 1, fp);

    str_assign(item->m_path_to_root, "");
    for(i = 0; i < len; i++) {
        if((c = getc(fp)) == 0) { /* replace 0 with path seperator */
            str_concat_chr(item->m_path_to_root, PATH_SEP[0]);
        } else {
            str_concat_chr(item->m_path_to_root, c);
        }
    }
    return;
}

static inline void formalize_path(string_t* path) {
    while(path->len > 0 && path->str[path->len - 1] == PATH_SEP[0]) {
        str_substr(path, 0, path->len - 2);
    }
    return;
}

static int traverse_directory_make_header(const char* path, const char* root, FILE* fp) {
    string_t* fulldir_path = str_new_with(path);
    string_t* rootdir_path = str_new_with(root);
    string_t* full_path;
    string_t* root_path;
    struct file_item* item = file_item_new();
    DIR* dir;
    struct dirent* dirent;

    formalize_path(fulldir_path);
    formalize_path(rootdir_path);

    /* write file item of root directory */
    if(!file_item_from_path(item, fulldir_path, rootdir_path)) {
        fprintf(stderr, "warning: cannot stat '%s'.\n", fulldir_path->str);
        file_item_del(item);
        str_del(fulldir_path);
        str_del(rootdir_path);
        return -1;
    }
    file_item_put(item, fp);

    /* open directory */
    if((dir = opendir(fulldir_path->str)) == NULL) {
        fprintf(stderr, "warning: cannot open directory '%s'.\n", fulldir_path->str);
        file_item_del(item);
        str_del(fulldir_path);
        str_del(rootdir_path);
        return -1;
    }

    full_path = str_new();
    root_path = str_new();
    while((dirent = readdir(dir)) != NULL) {
        if(strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0) {
            continue;
        }
        str_sprintf(full_path, PATH_JOINER, fulldir_path->str, dirent->d_name); /* reach a sub file/dir */
        str_sprintf(root_path, PATH_JOINER, rootdir_path->str, dirent->d_name);

        if(!file_item_from_path(item, full_path, root_path)) { /* get file item */
            fprintf(stderr, "warning: cannot stat '%s'.\n", full_path->str);
            continue;
        }
        if(S_ISREG(item->m_mode)) { /* write file item of a file */
            file_item_put(item, fp);
        }
        if(S_ISDIR(item->m_mode)) { /* traverse into a sub directory */
            traverse_directory_make_header(full_path->str, root_path->str, fp);
        }
    }
    file_item_del(item);
    closedir(dir);

    str_del(full_path);
    str_del(root_path);
    str_del(fulldir_path);
    str_del(rootdir_path);
    return 0;
}

static inline int encode_block(buf_t* ib, ppm_model_t* ppm, FILE* fout) {
    buf_t* ob = buf_new();
    size_t pos_sync;
    size_t out_size;

    /* put block header */
    pos_sync = ftell(fout);
    if(fwrite(&ob->m_size, sizeof(ob->m_size), 1, fout) != 1) {
        buf_del(ob);
        return -1;
    }

    /* encode */
    out_size = slimox_encode(ib, ob, ppm, fout);

    /* rewrite block header */
    fseek(fout, pos_sync, SEEK_SET);
    if(fwrite(&out_size, sizeof(ob->m_size), 1, fout) != 1) {
        buf_del(ob);
        return -1;
    }
    fseek(fout, 0, SEEK_END);

    buf_del(ob);
    return 0;
}

static inline int decode_block(buf_t* ob, ppm_model_t* ppm, FILE* fin) {
    buf_t* ib = buf_new();

    /* read block header */
    if(fread(&ib->m_size, sizeof(ib->m_size), 1, fin) != 1) {
        buf_del(ib);
        return -1;
    }
    buf_resize(ib, ib->m_size);

    /* read block data */
    if(fread(ib->m_data, ib->m_size, 1, fin) != 1) {
        buf_del(ib);
        return -1;
    }

    /* decode */
    buf_resize(ob, 0);
    slimox_decode(ib, ob, ppm);

    buf_del(ib);
    return 0;
}

int pack(const char* path, const char* outfile, ppm_model_t* ppm, int block_size) {
    FILE* ftmp;
    FILE* fin;
    FILE* fout;
    struct file_item* item;
    size_t in_size;
    int c;
    string_t* full_path;
    string_t* file_path;
    buf_t* ib;

    if((fout = fopen(outfile, "wb")) == NULL) {
        fprintf(stderr, "error: cannot open '%s'.\n", outfile);
        return -1;
    }
    if((ftmp = tmpfile()) == NULL) {
        fprintf(stderr, "error: cannot create temporary file.\n");
        fclose(fout);
        return -1;
    }

    /* write items into temporary file */
    if(traverse_directory_make_header(path, "", ftmp) == -1) {
        fprintf(stderr, "error: cannot open root directory '%s'.\n", path);
        fclose(ftmp);
        fclose(fout);
        return -1;
    }
    fprintf(stderr, "packing directory %s to %s...\n", path, outfile);

    item = file_item_new();
    item->m_mode = -1;
    item->m_size = -1;
    file_item_put(item, ftmp);
    file_item_del(item);

    /* copy items to out file */
    rewind(ftmp);
    while((c = fgetc(ftmp)) != EOF) {
        fputc(c, fout);
    }

    full_path = str_new_with(path);
    file_path = str_new();
    formalize_path(full_path);

    item = file_item_new();
    ib = buf_new();
    buf_resize(ib, block_size);
    ib->m_size = 0;

    /* compress file to out file */
    rewind(ftmp);
    while(file_item_get(item, ftmp), item->m_size != -1) {
        if(S_ISREG(item->m_mode)) {
            str_sprintf(file_path, PATH_JOINER, full_path->str, item->m_path_to_root->str);

            if((fin = fopen(file_path->str, "rb")) == NULL) {
                fprintf(stderr, "warning: cannot open '%s'.\n", file_path->str);
                continue;
            }

            while((in_size = fread(ib->m_data + ib->m_size, 1, ib->m_capacity - ib->m_size, fin)) > 0) {
                if((ib->m_size += in_size) == ib->m_capacity) {
                    if(encode_block(ib, ppm, fout) == -1) {
                        fprintf(stderr, "error: encode failed.\n");
                        ib->m_size = 0;
                        break;
                    }
                    ib->m_size = 0;
                }
            }
            if(ftell(fin) != item->m_size) { /* file size changed during compression -- connot continue */
                fprintf(stderr, "error: '%s' file size changed during compression.\n", file_path->str);
                fclose(fout);
                file_item_del(item);
                buf_del(ib);
                str_del(full_path);
                str_del(file_path);
                return -1;
            }
            fclose(fin);
        }
    }
    if(ib->m_size > 0) { /* encode last block */
        if(encode_block(ib, ppm, fout) == -1) {
            fprintf(stderr, "error: encode failed.\n");
        }
    }

    fclose(ftmp);
    fclose(fout);
    file_item_del(item);
    buf_del(ib);
    str_del(full_path);
    str_del(file_path);
    return 0;
}

int unpack(const char* infile, const char* path, ppm_model_t* ppm, int block_size) {
    FILE* ftmp;
    FILE* fin;
    FILE* fout;
    struct file_item* item;
    int nitems = 0;
    string_t* full_path;
    string_t* file_path;
    buf_t* ob;
    size_t ob_offset = 0;
    size_t ob_write_size = 0;

    if((fin = fopen(infile, "rb")) == NULL) {
        fprintf(stderr, "error: cannot open '%s'.\n", infile);
        return -1;
    }
    if((ftmp = tmpfile()) == NULL) {
        fprintf(stderr, "error: cannot create temporary file.\n");
        fclose(fin);
        return -1;
    }

    item = file_item_new();
    full_path = str_new_with(path);
    file_path = str_new();
    formalize_path(full_path);

    /* read items into temporary file */
    while(file_item_get(item, fin), item->m_size != -1) {
        file_item_put(item, ftmp);
        nitems += 1;
    }

    /* compress file to out file */
    ob = buf_new();
    rewind(ftmp);
    while(nitems > 0) {
        file_item_get(item, ftmp);
        nitems -= 1;
        str_sprintf(file_path, PATH_JOINER, full_path->str, item->m_path_to_root->str);

        if(S_ISDIR(item->m_mode)) {
            mkdir(file_path->str, item->m_mode); /* create directory */

        } else {
            if((fout = fopen(file_path->str, "wb")) == NULL) {
                fprintf(stderr, "warning: cannot open file '%s'.\n", file_path->str);
                continue;
            }
            while(item->m_size > 0) { /* write file */
                if(ob_offset == ob->m_size) {
                    ob_offset = 0;
                    if(decode_block(ob, ppm, fin) == -1) {
                        fprintf(stderr, "error: decode failed.\n");
                        fclose(fout);
                        break;
                    }
                }
                if(item->m_size < ob->m_size - ob_offset) {
                    ob_write_size = item->m_size;
                } else {
                    ob_write_size = ob->m_size - ob_offset;
                }
                fwrite(ob->m_data + ob_offset, 1, ob_write_size, fout);
                item->m_size -= ob_write_size;
                ob_offset += ob_write_size;
            }
            fchmod(fileno(fout), item->m_mode);
            fclose(fout);
        }
    }

    fclose(fin);
    file_item_del(item);
    buf_del(ob);
    str_del(full_path);
    str_del(file_path);
    return 0;
}
