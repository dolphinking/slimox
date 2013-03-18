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
#ifndef HEADER_STRING_UTILS_H
#define HEADER_STRING_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifndef STRING_UTILS_ON_ALLOC_FAIL
#define STRING_UTILS_ON_ALLOC_FAIL() (fprintf(stderr, "fatal: allocation failed.\n"), abort()) /* default handler */
#endif

/*******************************************************************************
 * Interface
 ******************************************************************************/
typedef struct string_t {
    size_t len;
    size_t capacity;
    char*  str;
} string_t;

/* ctor */
static inline string_t* str_new();
static inline string_t* str_new_with(const char* s);

/* dtor */
static inline void str_del(string_t* str);

/* assign */
static inline void str_assign(string_t* str, const char* s);

/* substr */
static inline void str_substr(string_t* str, size_t from, size_t to);

/* concatence */
static inline void str_concat_str(string_t* str, const char* s);
static inline void str_concat_chr(string_t* str, int c);

/* find */
static inline size_t str_find_first(string_t* str, const char* s);
static inline size_t str_find_last(string_t* str, const char* s);

/* replace */
static inline size_t str_replace_first(string_t* str, const char* oldstr, const char* newstr);
static inline size_t str_replace_last(string_t* str, const char* oldstr, const char* newstr);
static inline int    str_replace_all(string_t* str, const char* oldstr, const char* newstr);
static inline void   str_replace_range(string_t* str, size_t from, size_t to, const char* newstr);

/* formatting */
static inline int str_sprintf(string_t* str, const char* format, ...);

/*******************************************************************************
 * Implementation
 ******************************************************************************/
static inline string_t* str_new() {
    string_t* str;

    if((str = malloc(sizeof(string_t))) != NULL) {
        if((str->str = malloc(4)) != NULL) {
            str->str[0] = 0;
            str->len = 0;
            str->capacity = 1;
            return str;
        }
        free(str);
    }
    STRING_UTILS_ON_ALLOC_FAIL();
    return NULL;
}

static inline string_t* str_new_with(const char* s) {
    string_t* str;
    size_t slen = strlen(s);

    if((str = malloc(sizeof(string_t))) != NULL) {
        if((str->str = malloc(slen + 4)) != NULL) {
            strcpy(str->str, s);
            str->len = slen;
            str->capacity = slen + 4;
            return str;
        }
        free(str);
    }
    STRING_UTILS_ON_ALLOC_FAIL();
    return NULL;
}

static inline void str_del(string_t* str) {
    free(str->str);
    free(str);
    return;
}

static inline void str_assign(string_t* str, const char* s) {
    char* newp;
    size_t slen = strlen(s);

    if((newp = realloc(str->str, slen + 4)) != NULL) {
        str->str = newp;
        strcpy(str->str, s);
        str->len = slen;
        str->capacity = slen + 4;
        return;
    }
    STRING_UTILS_ON_ALLOC_FAIL();
    return;
}

static inline void str_substr(string_t* str, size_t from, size_t to) {
    str->len = to - from + 1;
    memmove(str->str, str->str + from, str->len);
    str->str[str->len] = 0;
    return;
}

static inline void str_concat_str(string_t* str, const char* s) {
    size_t newcapacity;
    void*  newp;
    size_t slen = strlen(s);

    if(str->len + slen + 1 > str->capacity) {
        newcapacity = str->len * 2 + slen + 4;
        if((newp = realloc(str->str, newcapacity)) != NULL) {
            str->capacity = newcapacity;
            str->str = newp;
        } else {
            STRING_UTILS_ON_ALLOC_FAIL();
            return;
        }
    }
    strcpy(str->str + str->len, s);
    str->len += slen;
    return;
}

static inline void str_concat_chr(string_t* str, int c) {
    size_t newcapacity;
    void*  newp;

    if(str->len + 2 > str->capacity) {
        newcapacity = str->len * 2 + 4;
        if((newp = realloc(str->str, newcapacity)) != NULL) {
            str->capacity = newcapacity;
            str->str = newp;
        } else {
            STRING_UTILS_ON_ALLOC_FAIL();
            return;
        }
    }
    str->str[str->len] = c, str->len++;
    str->str[str->len] = 0;
    return;
}

static inline size_t str_find_first(string_t* str, const char* s) {
    size_t slen = strlen(s);
    size_t spos;

    if(slen <= str->len) {
        for(spos = 0; spos <= str->len - slen; spos++) {
            if(memcmp(s, str->str + spos, slen) == 0) {
                return spos;
            }
        }
    }
    return -1;
}

static inline size_t str_find_last(string_t* str, const char* s) {
    size_t slen = strlen(s);
    size_t spos;

    if(slen <= str->len) {
        for(spos = str->len - slen; spos != -1; spos--) {
            if(memcmp(s, str->str + spos, slen) == 0) {
                return spos;
            }
        }
    }
    return -1;
}

static inline size_t str_replace_first(string_t* str, const char* oldstr, const char* newstr) {
    size_t oldlen = strlen(oldstr);
    size_t oldpos = str_find_first(str, oldstr);

    if(oldpos != -1) {
        str_replace_range(str, oldpos, oldpos + oldlen - 1, newstr);
        return oldpos;
    }
    return -1;
}

static inline size_t str_replace_last(string_t* str, const char* oldstr, const char* newstr) {
    size_t oldlen = strlen(oldstr);
    size_t oldpos = str_find_last(str, oldstr);

    if(oldpos != -1) {
        str_replace_range(str, oldpos, oldpos + oldlen - 1, newstr);
        return oldpos;
    }
    return -1;
    return -1;
}

static inline int str_replace_all(string_t* str, const char* oldstr, const char* newstr) {
    size_t oldlen = strlen(oldstr);
    size_t newlen = strlen(newstr);
    size_t oldpos;
    int num_replace = 0;

    if(oldlen <= str->len) {
        for(oldpos = 0; oldpos <= str->len - oldlen; oldpos++) {
            if(memcmp(oldstr, str->str + oldpos, oldlen) == 0) {
                num_replace++;
                str_replace_range(str, oldpos, oldpos + oldlen - 1, newstr);
                oldpos += newlen - 1;
            }
        }
    }
    return num_replace;
}

static inline void str_replace_range(string_t* str, size_t from, size_t to, const char* newstr) {
    size_t oldlen = to - from + 1;
    size_t newlen = strlen(newstr);
    size_t newcapacity;
    void*  newp;

    if(str->len + newlen - oldlen + 1 > str->capacity) {
        newcapacity = str->len * 2 + newlen - oldlen + 4;
        if((newp = realloc(str->str, newcapacity)) != NULL) {
            str->capacity = newcapacity;
            str->str = newp;
        } else {
            STRING_UTILS_ON_ALLOC_FAIL();
            return;
        }
    }
    memmove(str->str + from + newlen, str->str + to + 1, str->len - to);
    memcpy(str->str + from, newstr, newlen);
    str->len -= oldlen;
    str->len += newlen;
    return;
}

static inline int str_sprintf(string_t* str, const char* format, ...) {
    va_list args[2];
    size_t newcapacity;
    void*  newp;
    int    ret;

    va_start(args[0], format);
    ret = vsnprintf(str->str, str->capacity, format, args[0]);
    va_end(args[0]);

    if((newcapacity = ret + 4) > str->capacity) {
        if((newp = realloc(str->str, newcapacity)) != NULL) {
            str->capacity = newcapacity;
            str->str = newp;

            va_start(args[1], format);
            ret = vsnprintf(str->str, str->capacity, format, args[0]);
            va_end(args[1]);
        } else {
            STRING_UTILS_ON_ALLOC_FAIL();
            return -1;
        }
    }
    str->len = strlen(str->str);

    va_end(args[0]);
    return ret;
}

/*******************************************************************************
 * Test
 ******************************************************************************/
#ifdef STRING_UTILS_INCLUDE_TEST
#include <assert.h>

static inline int __str_test() {
    string_t* s1;
    string_t* s2;

    fprintf(stderr, "test str_new() and str_del()...\n"); {
        s1 = str_new();
        s2 = str_new();
        assert(s1->len == strlen(s1->str));
        assert(s2->len == strlen(s2->str));
        assert(strcmp(s1->str, "") == 0);
        assert(strcmp(s2->str, "") == 0);
        str_del(s1);
        str_del(s2);
    }

    fprintf(stderr, "test str_new_with()...\n"); {
        s1 = str_new_with("test1");
        s2 = str_new_with("");
        assert(s1->len == strlen(s1->str));
        assert(s2->len == strlen(s2->str));
        assert(strcmp(s1->str, "test1") == 0);
        assert(strcmp(s2->str, "") == 0);
        str_del(s1);
        str_del(s2);
    }

    fprintf(stderr, "test str_assign()...\n"); {
        s1 = str_new_with("test1");
        s2 = str_new_with("test2");
        str_assign(s1, "str_assign()_test1");
        str_assign(s2, "str_assign()_test2");
        assert(s1->len == strlen(s1->str));
        assert(s2->len == strlen(s2->str));
        assert(strcmp(s1->str, "str_assign()_test1") == 0);
        assert(strcmp(s2->str, "str_assign()_test2") == 0);
        str_del(s1);
        str_del(s2);
    }

    fprintf(stderr, "test str_substr()...\n"); {
        s1 = str_new_with("substr_test1_");
        s2 = str_new_with("substr_test2_");
        str_substr(s1, 7, 11);
        str_substr(s2, 7, 11);
        assert(s1->len == strlen(s1->str));
        assert(s2->len == strlen(s2->str));
        assert(strcmp(s1->str, "test1") == 0);
        assert(strcmp(s2->str, "test2") == 0);
        str_del(s1);
        str_del(s2);
    }

    fprintf(stderr, "test str_concat_str()...\n"); {
        s1 = str_new_with("test1_");
        s2 = str_new_with("test2_");
        str_concat_str(s1, "concat_str");
        str_concat_str(s2, "concat_str");
        assert(s1->len == strlen(s1->str));
        assert(s2->len == strlen(s2->str));
        assert(strcmp(s1->str, "test1_concat_str") == 0);
        assert(strcmp(s2->str, "test2_concat_str") == 0);
        str_del(s1);
        str_del(s2);
    }

    fprintf(stderr, "test str_concat_chr()...\n"); {
        s1 = str_new_with("test1_");
        s2 = str_new_with("test2_");
        str_concat_chr(s1, 'c');
        str_concat_chr(s2, 'c');
        assert(s1->len == strlen(s1->str));
        assert(s2->len == strlen(s2->str));
        assert(strcmp(s1->str, "test1_c") == 0);
        assert(strcmp(s2->str, "test2_c") == 0);
        str_del(s1);
        str_del(s2);
    }

    fprintf(stderr, "test str_find_first()...\n"); {
        s1 = str_new_with("$1$1$2$2$3$3_$$");
        assert(str_find_first(s1, "$1") == 0);
        assert(str_find_first(s1, "$2") == 4);
        assert(str_find_first(s1, "$3") == 8);
        assert(str_find_first(s1, "$$") == 13);
        assert(str_find_first(s1, "$4") == -1);
        str_del(s1);
    }

    fprintf(stderr, "test str_find_last()...\n"); {
        s1 = str_new_with("$^_$1$1$2$2$3$3");
        assert(str_find_last(s1, "$1") == 5);
        assert(str_find_last(s1, "$2") == 9);
        assert(str_find_last(s1, "$3") == 13);
        assert(str_find_last(s1, "$^") == 0);
        assert(str_find_last(s1, "$4") == -1);
        str_del(s1);
    }

    fprintf(stderr, "test str_replace_range()...\n"); {
        s1 = str_new_with("test_replace_range");
        s2 = str_new_with("test_replace_range");
        str_replace_range(s1, 5, 11, "x");
        str_replace_range(s2, 5, 11, "y");
        assert(s1->len == strlen(s1->str));
        assert(s2->len == strlen(s2->str));
        assert(strcmp(s1->str, "test_x_range") == 0);
        assert(strcmp(s2->str, "test_y_range") == 0);
        str_del(s1);
        str_del(s2);
    }

    fprintf(stderr, "test str_replace_first()...\n"); {
        s1 = str_new_with("$1$1$2$2$3$3_$$"); /* => "aaa$1bbb$2ccc$3_ddd" */
        assert(str_replace_first(s1, "$1", "aaa") == 0);
        assert(str_replace_first(s1, "$2", "bbb") == 5);
        assert(str_replace_first(s1, "$3", "ccc") == 10);
        assert(str_replace_first(s1, "$$", "ddd") == 16);
        assert(str_replace_first(s1, "$4", "???") == -1);
        assert(s1->len == strlen(s1->str));
        assert(strcmp(s1->str, "aaa$1bbb$2ccc$3_ddd") == 0);
        str_del(s1);
    }

    fprintf(stderr, "test str_replace_last()...\n"); {
        s1 = str_new_with("$^_$1$1$2$2$3$3"); /* -> "ddd_$1aaa$2bbb$3ccc" */
        assert(str_replace_last(s1, "$^", "ddd") == 0);
        assert(str_replace_last(s1, "$1", "aaa") == 6);
        assert(str_replace_last(s1, "$2", "bbb") == 11);
        assert(str_replace_last(s1, "$3", "ccc") == 16);
        assert(str_replace_last(s1, "$4", "???") == -1);
        assert(s1->len == strlen(s1->str));
        assert(strcmp(s1->str, "ddd_$1aaa$2bbb$3ccc") == 0);
        str_del(s1);
    }

    fprintf(stderr, "test str_replace_all()...\n"); {
        s1 = str_new_with("$1$1$2$2$3$3"); /* -> "aaaaaabbbbbbcccccc" */
        assert(str_replace_all(s1, "$1", "aaa") == 2);
        assert(str_replace_all(s1, "$2", "bbb") == 2);
        assert(str_replace_all(s1, "$3", "ccc") == 2);
        assert(str_replace_all(s1, "$4", "???") == 0);
        assert(s1->len == strlen(s1->str));
        assert(strcmp(s1->str, "aaaaaabbbbbbcccccc") == 0);
        str_del(s1);
    }

    fprintf(stderr, "test str_sprintf()...\n"); {
        s1 = str_new_with("test1");
        s2 = str_new_with("test2");
        str_sprintf(s1, "test_%02d_%.7s", 1, "sprintf_world");
        str_sprintf(s2, "test_%02d_%.7s", 2, "sprintf_world");
        assert(s1->len == strlen(s1->str));
        assert(s2->len == strlen(s2->str));
        assert(strcmp(s1->str, "test_01_sprintf") == 0);
        assert(strcmp(s2->str, "test_02_sprintf") == 0);
        str_del(s1);
        str_del(s2);
    }
    return 0;
}
#endif /* STRING_UTILS_INCLUDE_TEST */
#endif
