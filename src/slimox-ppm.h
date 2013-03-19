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
#ifndef HEADER_SLIMOX_PPM_H
#define HEADER_SLIMOX_PPM_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include "slimox-alloc.h"
#include "slimox-rc.h"

#ifndef __param
#define __param(t, p, n) t(__param_##n)=(p); t(n)=(__param_##n)
#endif

#define min2(a, b) ((a)<=(b) ? (a):(b))
#define max2(a, b) ((a)>=(b) ? (a):(b))
#define swap(a, b) do{int (_)=(a); (a)=(b); (b)=(_);}while(0)

/* secondary escape elimination type */
typedef struct see_model_t {
    uint16_t m_c[2];
} see_model_t;

#define __context_sym(x, i) ((x)->m_symbols[i][0])
#define __context_frq(x, i) ((x)->m_symbols[i][1])

/* o2-structure types */
typedef struct o2_context_t {
    uint16_t m_sum;
    uint16_t m_cnt;
    uint16_t m_esc;
    uint8_t  m_symbols[256][2];
} __attribute__((__packed__)) o2_context_t;

/* o4-structure types */
typedef struct o4_context_t {
    struct o4_context_t* m_next;
    uint8_t  m_padding[8 - sizeof(size_t)]; /* make a 128-byte struct */
    uint32_t m_context;
    uint16_t m_sum;
    uint8_t  m_cnt;
    uint8_t  m_visited;
    uint8_t  m_symbols[66][2]; /* can store 66 symbols at most */
} __attribute__((__packed__)) o4_context_t;

/* main ppm-model type */
typedef struct ppm_model_t {
    uint32_t m_context;
    uint32_t m_o2_counts;
    uint32_t m_o4_counts;
    struct o4_context_t* m_o4_buckets[262144];
    struct o2_context_t* m_o2[65536];
    struct o2_context_t  m_o1[256];
    struct o2_context_t  m_o0[1];
    struct allocator_t*  m_o2_allocator;
    struct allocator_t*  m_o4_allocator;
    uint8_t m_sse_ch_context;
    uint8_t m_sse_last_esc;
    struct see_model_t m_see[131072];
    struct see_model_t m_see_01;
    struct see_model_t m_see_10;
} ppm_model_t;

static inline ppm_model_t* ppm_model_new() {
    static const int see_init[128][2] = {
        {200, 200}, {173, 200}, {200, 200}, {200, 200}, {200, 200}, {200, 103}, {197, 200}, {200, 200},
        {200, 200}, {200,  51}, {200, 102}, {200, 173}, {200, 200}, {200,  25}, {200,  47}, {200,  96},
        {200, 200}, {200,  18}, {200,  24}, {200,  49}, {200, 200}, {200,  11}, {200,  13}, {200,  24},
        {200, 200}, {200,  17}, {200,   8}, {200,  12}, {200, 200}, {200, 194}, {200,  15}, {200,   7},
        {200, 200}, {200, 169}, {200, 200}, {200, 200}, {200, 200}, {200,  73}, {200, 148}, {200, 200},
        {200, 200}, {200,  33}, {200,  68}, {200, 135}, {200, 200}, {200,  13}, {200,  32}, {200,  59},
        {200, 200}, {200,   7}, {200,  14}, {200,  29}, {200, 200}, {200,   3}, {200,   4}, {200,  12},
        {200, 200}, {200,   3}, {200,   3}, {200,   5}, {200, 200}, {200,  55}, {200,   3}, {200,   3},

        {200, 200}, {164, 200}, {121, 200}, { 73, 200}, {200,  90}, {200,  96}, {200, 136}, {200, 171},
        {200,  23}, {200,  29}, {200,  76}, {200, 102}, {200,  37}, {200,  17}, {200,  50}, {200,  61},
        {200,  62}, {200,  15}, {200,  38}, {200,  42}, {200,  99}, {200,  22}, {200,  48}, {200,  45},
        {200, 142}, {200,  26}, {200,  67}, {200,  58}, {200, 194}, {200,  77}, {200, 138}, {200,  82},
        {200, 200}, {200, 168}, {200, 193}, {132, 200}, {200,  57}, {200,  61}, {200,  98}, {200, 119},
        {200,  18}, {200,  25}, {200,  48}, {200,  59}, {200,  14}, {200,  17}, {200,  26}, {200,  30},
        {200,  15}, {200,  11}, {200,  15}, {200,  17}, {200,  15}, {200,   9}, {200,  11}, {200,  11},
        {200,  18}, {200,   6}, {200,   8}, {200,   6}, {200,  32}, {200,   8}, {200,   7}, {200,   6},
    };
    int context_hi;
    int context_lo;
    ppm_model_t* model = (ppm_model_t*)malloc(sizeof(ppm_model_t));

    model->m_o2_counts = 0;
    model->m_o4_counts = 0;
    model->m_context = 0;
    memset(model->m_o4_buckets, 0, sizeof(model->m_o4_buckets));
    memset(model->m_o2, 0, sizeof(model->m_o2));
    memset(model->m_o1, 0, sizeof(model->m_o1));
    memset(model->m_o0, 0, sizeof(model->m_o0));
    model->m_o4_allocator = allocator_new();
    model->m_o2_allocator = allocator_new();
    model->m_sse_ch_context = 0;
    for(context_hi = 0; context_hi < 128; context_hi++) {
        for(context_lo = 0; context_lo < 1024; context_lo++) {
            model->m_see[context_hi << 10 | context_lo].m_c[0] = see_init[context_hi][0];
            model->m_see[context_hi << 10 | context_lo].m_c[1] = see_init[context_hi][1];
        }
    }
    model->m_see_01.m_c[0] = 0;
    model->m_see_01.m_c[1] = 1;
    model->m_see_10.m_c[0] = 1;
    model->m_see_10.m_c[1] = 0;
    return model;
}

static inline void ppm_model_del(ppm_model_t* model) {
    allocator_del(model->m_o4_allocator);
    allocator_del(model->m_o2_allocator);
    free(model);
    return;
}

#define __o4_hash(c)    ((unsigned)((c) ^ (c) >> 14) % 262144)
#define __o2(model)     ( (model)->m_o2[(model)->m_context & 0xffff])
#define __o1(model)     (&(model)->m_o1[(model)->m_context & 0x00ff])
#define __o0(model)     (&(model)->m_o0[0])
#define __ex_get(n)     ((__exclude[(n) / 32] >> ((n) % 32)) & 0x01)
#define __ex_set(n)     ((__exclude[(n) / 32] |= 0x01<< ((n) % 32)))

/* SEE model */
static inline void __ppm_see_update(see_model_t* see, int c, int update) {
    if((see->m_c[c] += update) > 16000) {
        see->m_c[0] = (see->m_c[0] + 1) / 2;
        see->m_c[1] = (see->m_c[1] + 1) / 2;
    }
    return;
}

/* SEE context quantilizer */
static inline int __fast_log2(uint32_t x) __attribute__((const));
static inline int __fast_log2(uint32_t x) {
    return (31 - __builtin_clz((x << 1) | 0x01));
}
static inline see_model_t* __ppm_see_context_o4(ppm_model_t* model, o4_context_t* o4) {
    int curcnt = o4->m_cnt;
    int lowsum = __o2(model)->m_sum;
    int lowcnt = __o2(model)->m_cnt;
    uint32_t context = 0;

    if(curcnt == 0) return &model->m_see_01; /* no symbols under current context -- always escape */
    if(curcnt == 1) {
        /* QUANTIZE(binary) = (last_esc[1] | sum[3] | lowcnt[2] | lowsum[1] | bin_symbol[3] | previous symbols[6]) */
        context |= ((model->m_context >>  6) & 0x03);
        context |= ((model->m_context >> 14) & 0x03) << 2;
        context |= ((model->m_context >> 22) & 0x03) << 4;
        context |= (__context_sym(o4, 0) >> 6) << 6;
        context |= (lowsum >= 4) << 9;
        context |= min2(__fast_log2(lowcnt / 2), 3) << 10;
        context |= min2(__fast_log2(o4->m_sum / 3), 7) << 12;
        context |= model->m_sse_last_esc << 15;
        context |= 1 << 16;
        return &model->m_see[context];
    } else {
        /* QUANTIZE = (last_esc[1] | sum[3] | curcnt[2] | lowsum[1] | (lowcnt - curcnt)[3] | previous symbols[6]) */
        context |= ((model->m_context >>  6) & 0x03);
        context |= ((model->m_context >> 14) & 0x03) << 2;
        context |= ((model->m_context >> 22) & 0x03) << 4;
        context |= min2(__fast_log2(max2(lowcnt - curcnt, 0) / 2), 3) << 6;
        context |= (lowsum >= 4) << 9;
        context |= min2(__fast_log2(curcnt / 2), 3) << 10;
        context |= min2(__fast_log2(o4->m_sum / 8), 7) << 12;
        context |= model->m_sse_last_esc << 15;
        context |= 0 << 16;
        return &model->m_see[context];
    }
    return NULL;
}

#define __o2_encode(o2, coder, c, do_output, escape, has_exclusion)     do{ \
    __param(o2_context_t*, o2, __o2);                                       \
    __param(rc_coder_t*, coder, __coder);                                   \
    __param(unsigned char, c, __c);                                         \
    __param(int*, escape, __escape);                                        \
    __param(int*, has_exclusion, __has_exclusion);                          \
    int __found = 0;                                                        \
    int __found_index = 0;                                                  \
    int __i;                                                                \
    uint16_t __cumx = 0;                                                    \
    uint16_t __frqx = 0;                                                    \
    uint16_t __sumx = 0;                                                    \
    uint16_t __escx = 0;                                                    \
    uint16_t __recent_frq = __context_frq(o2, 0) & -!__ex_get(__context_sym(o2, 0)); \
    if(!__has_exclusion[0]) {                                               \
        for(__i = 0; __i < __o2->m_cnt; __i++) { /*no exclusion */          \
            if(__context_sym(o2, __i) == __c) {                             \
                __found_index = __i;                                        \
                __found = 1;                                                \
                swap(__context_frq(o2, __i), __context_frq(o2, 0));         \
                swap(__context_sym(o2, __i), __context_sym(o2, 0));         \
                break;                                                      \
            }                                                               \
            __cumx += __context_frq(o2, __i);                               \
        }                                                                   \
        __sumx = __o2->m_sum;                                               \
    } else {                                                                \
        for(__i = 0; __i < __o2->m_cnt; __i++) {                            \
            __cumx += __context_frq(o2, __i) & -(!__ex_get(__context_sym(o2, __i)) && !__found && __context_sym(o2, __i) != __c); \
            __sumx += __context_frq(o2, __i) & -(!__ex_get(__context_sym(o2, __i))); \
            if(__context_sym(o2, __i) == __c) {                             \
                __found_index = __i;                                        \
                __found = 1;                                                \
                swap(__context_frq(o2, __i), __context_frq(o2, 0));         \
                swap(__context_sym(o2, __i), __context_sym(o2, 0));         \
            }                                                               \
        }                                                                   \
    }                                                                       \
    __frqx = __context_frq(o2, 0) + (__recent_frq & -(__found_index == 0)); \
    __escx = __o2->m_esc + !__o2->m_esc;                                    \
    __cumx += __recent_frq & -(__found_index > 0);                          \
    __sumx += __recent_frq + __escx;                                        \
    if(!__found) {                                                          \
        __escape[0] = 1;                                                    \
        for(__i = 0; __i < __o2->m_cnt; __i++) { /* exclude */              \
            __ex_set(__context_sym(o2, __i));                               \
            __has_exclusion[0] = 1;                                         \
        }                                                                   \
        __context_frq(o2, __o2->m_cnt) = __context_frq(o2, 0);              \
        __context_sym(o2, __o2->m_cnt) = __context_sym(o2, 0);              \
        __context_sym(o2, 0) = __c;                                         \
        __context_frq(o2, 0) = 0;                                           \
        __o2->m_cnt += 1;                                                   \
        __cumx = __sumx - __escx;                                           \
        __frqx = __escx;                                                    \
    } else {                                                                \
        __escape[0] = 0;                                                    \
    }                                                                       \
    rc_encode(__coder, __cumx, __frqx, __sumx, do_output);                  \
}while(0)

#define __o2_decode(o2, coder, c, do_input, escape, has_exclusion)      do{ \
    __param(o2_context_t*, o2, __o2);                                       \
    __param(rc_coder_t*, coder, __coder);                                   \
    __param(unsigned char*, c, __c);                                        \
    __param(int*, escape, __escape);                                        \
    __param(int*, has_exclusion, __has_exclusion);                          \
    int __i;                                                                \
    uint16_t __cumx = 0;                                                    \
    uint16_t __frqx = 0;                                                    \
    uint16_t __sumx = 0;                                                    \
    uint16_t __escx = 0;                                                    \
    uint16_t __recent_frq = __context_frq(o2, 0) & -!__ex_get(__context_sym(o2, 0)); \
    if(!__has_exclusion) {                                                  \
        __sumx = __o4->m_sum; /* no exclusion */                            \
    } else {                                                                \
        for(__i = 0; __i < __o2->m_cnt; __i++) {                            \
            __sumx += __context_frq(o2, __i) & -!__ex_get(__context_sym(o2, __i)); \
        }                                                                   \
    }                                                                       \
    __escx = __o2->m_esc + !__o2->m_esc;                                    \
    __sumx += __recent_frq + __escx;                                        \
    rc_decode_cum(__coder, __sumx);                                         \
    if(__sumx - __escx <= __coder->m_decode_cum) {                          \
        __escape[0] = 1;                                                    \
        for(__i = 0; __i < __o2->m_cnt; __i++) { /* exclude */              \
            __ex_set(__context_sym(o2, __i));                               \
            __has_exclusion[0] = 1;                                         \
        }                                                                   \
        __context_frq(o2, __o2->m_cnt) = __context_frq(o2, 0);              \
        __context_sym(o2, __o2->m_cnt) = __context_sym(o2, 0);              \
        __context_frq(o2, 0) = 0;                                           \
        __o2->m_cnt += 1;                                                   \
        __cumx = __sumx - __escx;                                           \
        __frqx = __escx;                                                    \
    } else {                                                                \
        __escape[0] = 0;                                                    \
        __i = 0;                                                            \
        if(!__has_exclusion) {                                              \
            while(__cumx + __recent_frq + __context_frq(o2, __i) <= __coder->m_decode_cum) { /* no exclusion */ \
                __cumx += __context_frq(o2, __i);                           \
                __i++;                                                      \
            }                                                               \
        } else {                                                            \
            while(__cumx + __recent_frq + (__context_frq(o2, __i) & -!__ex_get(__context_sym(o2, __i))) <= __coder->m_decode_cum) { \
                __cumx += __context_frq(o2, __i) & -!__ex_get(__context_sym(o2, __i)); \
                __i++;                                                      \
            }                                                               \
        }                                                                   \
        __frqx = __context_frq(o2, __i);                                    \
        __c[0] = __context_sym(o2, __i);                                    \
        if(__i == 0) {                                                      \
            __frqx += __recent_frq;                                         \
        } else {                                                            \
            swap(__context_frq(o2, __i), __context_frq(o2, 0));             \
            swap(__context_sym(o2, __i), __context_sym(o2, 0));             \
            __cumx += __recent_frq;                                         \
        }                                                                   \
    }                                                                       \
    rc_decode(__coder, __cumx, __frqx, do_input);                           \
}while(0)

static inline void __o2_update(o2_context_t* o2, int c) {
    int n = 0;
    int i;

    __context_frq(o2, 0) += 1;
    __context_sym(o2, 0) = c;
    o2->m_sum += 1;
    o2->m_esc += (__context_frq(o2, 0) == 1) - (__context_frq(o2, 0) == 2);

    if(__context_frq(o2, 0) > 250) { /* rescale */
        o2->m_cnt = 0;
        o2->m_sum = 0;
        o2->m_esc = 0;
        for(i = 0; i + n < 256; i++) {
            if((__context_frq(o2, i) = __context_frq(o2, i + n) / 2) > 0) {
                __context_sym(o2, i) = __context_sym(o2, i + n);
                o2->m_cnt += 1;
                o2->m_sum += __context_frq(o2, i);
                o2->m_esc += __context_frq(o2, i) == 1;
            } else {
                n++;
                i--;
            }
        }
        memset(o2->m_symbols + o2->m_cnt, 0, sizeof(o2->m_symbols[0]) * (256 - o2->m_cnt));
    }
    return;
}

static inline o4_context_t* __o4_context_node(ppm_model_t* model) {
    o4_context_t* o4;
    o4_context_t* o4_prev;
    int i;

    if(model->m_o4_counts >= 1048576) { /* too many o4-context/symbol nodes */
        model->m_o4_counts = 0;
        for(i = 0; i < 262144; i++) {
            while(model->m_o4_buckets[i] && model->m_o4_buckets[i]->m_visited / 2 == 0) {
                o4 = model->m_o4_buckets[i];
                model->m_o4_buckets[i] = model->m_o4_buckets[i]->m_next;
                allocator_free(model->m_o4_allocator, o4);
            }
            for(o4 = model->m_o4_buckets[i]; o4 != NULL; o4 = o4->m_next) {
                if((o4->m_visited /= 2) == 0) {
                    o4_prev->m_next = o4->m_next;
                    allocator_free(model->m_o4_allocator, o4);
                    o4 = o4_prev;
                } else {
                    model->m_o4_counts += 1;
                    o4_prev = o4;
                }
            }
        }
    }

    o4 = model->m_o4_buckets[__o4_hash(model->m_context)];
    o4_prev = NULL;
    while(o4 && o4->m_context != model->m_context) { /* search for o4 context */
        o4_prev = o4;
        o4 = o4->m_next;
    }
    if(o4 != NULL) { /* found -- bring to front of linked-list */
        if(o4_prev != NULL) {
            o4_prev->m_next = o4->m_next;
            o4->m_next = model->m_o4_buckets[__o4_hash(model->m_context)];
            model->m_o4_buckets[__o4_hash(model->m_context)] = o4;
        }
    } else { /* not found -- create new node for context */
        o4 = allocator_alloc(model->m_o4_allocator, sizeof(o4_context_t), 8192);
        memset(o4->m_symbols, 0, sizeof(o4->m_symbols));
        o4->m_context = model->m_context;
        o4->m_sum = 0;
        o4->m_cnt = 0;
        o4->m_visited = 0;
        o4->m_next = model->m_o4_buckets[__o4_hash(model->m_context)];
        model->m_o4_counts += 1;
        model->m_o4_buckets[__o4_hash(model->m_context)] = o4;
    }
    o4->m_visited += (o4->m_visited < 255);
    return o4;
}

#define __o4_encode(model, o4, coder, c, do_output, escape, exclusion)  do{ \
    __param(ppm_model_t*, model, __model);                                  \
    __param(rc_coder_t*, coder, __coder);                                   \
    __param(o4_context_t*, o4, __o4);                                       \
    __param(uint8_t, c, __c);                                               \
    __param(int*, escape, __escape);                                        \
    __param(int*, exclusion, __exclusion);                                  \
    see_model_t* __see = __ppm_see_context_o4(__model, __o4);               \
    uint8_t      __tmp_symbol[2];                                           \
    uint16_t     __cum = 0;                                                 \
    uint16_t     __frq = 0;                                                 \
    uint16_t     __recent_frq = 0;                                          \
    int          __i;                                                       \
    for(__i = 0; __i < __o4->m_cnt && __context_sym(o4, __i) != __c; __i++) { /* search for symbol */ \
        __cum += __context_frq(o4, __i);                                    \
    }                                                                       \
    if(__i < __o4->m_cnt) { /* found -- bring to front of linked-list */    \
        rc_encode(__coder, 0, __see->m_c[0], __see->m_c[0] + __see->m_c[1], do_output); \
        __ppm_see_update(__see, 0, 35);                                     \
        __escape[0] = 0;                                                    \
        if(__o4->m_cnt != 1) { /* no need to encode binary context */       \
            __recent_frq = (__context_frq(__o4, 0) + 6) / 2; /* recency scaling */ \
            if(__i == 0) {                                                  \
                __frq = __context_frq(__o4, __i) + __recent_frq;            \
            } else {                                                        \
                __frq = __context_frq(__o4, __i);                           \
                __cum += __recent_frq;                                      \
                __tmp_symbol[0] = __context_sym(__o4, __i);                 \
                __tmp_symbol[1] = __context_frq(__o4, __i);                 \
                memmove(__o4->m_symbols + 1, __o4->m_symbols, sizeof(__o4->m_symbols[0]) * __i); \
                __context_sym(__o4, 0) = __tmp_symbol[0];                   \
                __context_frq(__o4, 0) = __tmp_symbol[1];                   \
            }                                                               \
            rc_encode(__coder, __cum, __frq, __o4->m_sum + __recent_frq, do_output); \
        }                                                                   \
    } else { /* not found -- create new node for sym */                     \
        rc_encode(__coder, __see->m_c[0], __see->m_c[1], __see->m_c[0] + __see->m_c[1], do_output); \
        __ppm_see_update(__see, 1, 35);                                     \
        __escape[0] = 1;                                                    \
        for(__i = 0; __i < __o4->m_cnt; __i++) {                            \
            __ex_set(__context_sym(__o4, __i)); /* exclude o4 */            \
            __exclusion[0] = 1;                                             \
        }                                                                   \
        if(__o4->m_cnt == 66) {                                             \
            __o4->m_sum -= __context_frq(__o4, __o4->m_cnt - 1);            \
        } else {                                                            \
            __o4->m_cnt += 1;                                               \
        }                                                                   \
        memmove(__o4->m_symbols + 1, __o4->m_symbols, sizeof(__o4->m_symbols[0]) * (__o4->m_cnt - 1)); \
        __context_sym(__o4, 0) = __c;                                       \
        __context_frq(__o4, 0) = 0;                                         \
    }                                                                       \
}while(0)

#define __o4_decode(model, o4, coder, c, do_input, escape, exclusion)   do{ \
    __param(ppm_model_t*, model, __model);                                  \
    __param(rc_coder_t*, coder, __coder);                                   \
    __param(o4_context_t*, o4, __o4);                                       \
    __param(uint8_t*, c, __c);                                              \
    __param(int*, escape, __escape);                                        \
    __param(int*, exclusion, __exclusion);                                  \
    see_model_t* __see = __ppm_see_context_o4(__model, __o4);               \
    uint8_t      __tmp_symbol[2];                                           \
    uint16_t     __cum = 0;                                                 \
    uint16_t     __frq = 0;                                                 \
    uint16_t     __recent_frq = 0;                                          \
    int          __i;                                                       \
    rc_decode_cum(__coder, __see->m_c[0] + __see->m_c[1]);                  \
    if(__see->m_c[0] <= __coder->m_decode_cum) {                            \
        rc_decode(__coder, __see->m_c[0], __see->m_c[1], do_input);         \
        __ppm_see_update(__see, 1, 35);                                     \
        __escape[0] = 1;                                                    \
        for(__i = 0; __i < __o4->m_cnt; __i++) {                            \
            __ex_set(__context_sym(__o4, __i)); /* exclude o4 */            \
            __exclusion[0] = 1;                                             \
        }                                                                   \
        if(__o4->m_cnt == 66) {                                             \
            __o4->m_sum -= __context_frq(__o4, __o4->m_cnt - 1);            \
        } else {                                                            \
            __o4->m_cnt += 1;                                               \
        }                                                                   \
        memmove(__o4->m_symbols + 1, __o4->m_symbols, sizeof(__o4->m_symbols[0]) * (__o4->m_cnt - 1)); \
        __context_frq(__o4, 0) = 0;                                         \
    } else { /* found */                                                    \
        rc_decode(__coder, 0, __see->m_c[0], do_input);                     \
        __ppm_see_update(__see, 0, 35);                                     \
        __escape[0] = 0;                                                    \
        if(__o4->m_cnt != 1) { /* no need to decode binary context */       \
            __recent_frq = (__context_frq(o4, 0) + 6) / 2; /* recency scaling */ \
            rc_decode_cum(__coder, __o4->m_sum + __recent_frq);             \
            __i = 0;                                                        \
            while(__cum + __recent_frq + __context_frq(o4, __i) <= __coder->m_decode_cum) { \
                __cum += __context_frq(o4, __i);                            \
                __i++;                                                      \
            }                                                               \
            if(__i == 0) {                                                  \
                __frq = __context_frq(__o4, __i) + __recent_frq;            \
            } else {                                                        \
                __frq = __context_frq(__o4, __i);                           \
                __cum += __recent_frq;                                      \
                __tmp_symbol[0] = __context_sym(__o4, __i);                 \
                __tmp_symbol[1] = __context_frq(__o4, __i);                 \
                memmove(__o4->m_symbols + 1, __o4->m_symbols, sizeof(__o4->m_symbols[0]) * __i); \
                __context_sym(__o4, 0) = __tmp_symbol[0];                   \
                __context_frq(__o4, 0) = __tmp_symbol[1];                   \
            }                                                               \
            rc_decode(__coder, __cum, __frq, do_input);                     \
        }                                                                   \
        __c[0] = __context_sym(o4, 0);                                      \
    }                                                                       \
}while(0)

static inline void __o4_update(ppm_model_t* model, o4_context_t* o4, int c) {
    o2_context_t* o2 = __o2(model);
    int n = 0;
    int i;
    int o2frq = 0;

    if(__context_frq(o4, 0) == 0) { /* calculate init frequency */
        for(i = 0; i < o2->m_cnt; i++) {
            if(__context_sym(o2, i) == c) {
                o2frq = __context_frq(o2, i);
                break;
            }
        }
        if(o4->m_sum == 0) {
            __context_frq(o4, 0) = 2 + o2frq * 2 / (o2->m_sum + o2->m_cnt + 1 - o2frq);
        } else {
            __context_frq(o4, 0) = 2 + o2frq * (o4->m_sum + o4->m_cnt + 1) / (o2->m_sum + o2->m_cnt + o4->m_sum - o2frq);
        }
        __context_frq(o4, 0) = (__context_frq(o4, 0) > 0) ? __context_frq(o4, 0) : 1;
        __context_frq(o4, 0) = (__context_frq(o4, 0) < 8) ? __context_frq(o4, 0) : 7;
        __context_sym(o4, 0) = c;
        o4->m_sum += __context_frq(o4, 0);
    } else {
        /* update freq */
        __context_frq(o4, 0) += 3;
        __context_sym(o4, 0) = c;
        o4->m_sum += 3;
    }

    if(__context_frq(o4, 0) > 250) { /* rescale */
        o4->m_cnt = 0;
        o4->m_sum = 0;
        for(i = 0; i + n < 66; i++) {
            if((__context_frq(o4, i) = __context_frq(o4, i + n) / 2) > 0) {
                __context_sym(o4, i) = __context_sym(o4, i + n);
                o4->m_cnt += 1;
                o4->m_sum += __context_frq(o4, i);
            } else {
                n++;
                i--;
            }
        }
        memset(o4->m_symbols + o4->m_cnt, 0, sizeof(o4->m_symbols[0]) * (66 - o4->m_cnt));
    }
    return;
}

/* main ppm-encode method */
#define ppm_encode(model, coder, c, do_output)                          do{ \
    __param(ppm_model_t*, model, __model);                                  \
    __param(rc_coder_t*, coder, __coder);                                   \
    __param(uint8_t, (uint8_t)(c), __c);                                    \
    o4_context_t* __o4 = __o4_context_node(__model);                        \
    int           __i;                                                      \
    int           __order = 2;                                              \
    int           __has_exclusion = 0;                                      \
    int           __escape;                                                 \
    uint16_t      __cum = 0;                                                \
    uint16_t      __sum = 0;                                                \
    uint32_t      __exclude[8] = {0};                                       \
    if(!__o2(__model)) {                                                    \
        __o2(__model) = allocator_alloc(__model->m_o2_allocator, sizeof(o2_context_t), 1024); \
        __model->m_o2_counts += 1;                                          \
        memset(__o2(__model), 0, sizeof(o2_context_t));                     \
    }                                                                       \
    __builtin_prefetch(__model->m_o4_buckets + __o4_hash(model->m_context << 8 | __c), 1, 3); /* prefetch for next round */ \
    while(-1) {                                                             \
        __order = 4; __o4_encode(__model, __o4, __coder, __c, do_output, &__escape, &__has_exclusion); if(!__escape) break; \
        __order = 2; __o2_encode(__o2(__model), __coder, __c, do_output, &__escape, &__has_exclusion); if(!__escape) break; \
        __order = 1; __o2_encode(__o1(__model), __coder, __c, do_output, &__escape, &__has_exclusion); if(!__escape) break; \
        __order = 0; __o2_encode(__o0(__model), __coder, __c, do_output, &__escape, &__has_exclusion); if(!__escape) break; \
        for(__i = 0; __i < 256; __i++) { /* encode with o(-1) */            \
            __cum += !__ex_get(__i) && (__i < __c);                         \
            __sum += !__ex_get(__i);                                        \
        }                                                                   \
        rc_encode(__coder, __cum, 1, __sum, do_output);                     \
        break;                                                              \
    }                                                                       \
    switch(__order) { /* fall-through switch */                             \
        case 0: __o2_update(__o0(__model), __c);                            \
        case 1: __o2_update(__o1(__model), __c);                            \
        case 2: __o2_update(__o2(__model), __c);                            \
        case 4: __o4_update(__model, __o4, __c);                            \
    }                                                                       \
    model->m_sse_last_esc = (__order == 4);                                 \
}while(0)

/* main ppm-decode method */
#define ppm_decode(model, coder, c, do_input)                           do{ \
    __param(ppm_model_t*, model, __model);                                  \
    __param(rc_coder_t*, coder, __coder);                                   \
    __param(uint8_t*, (uint8_t*)(c), __c);                                  \
    o4_context_t* __o4 = __o4_context_node(__model);                        \
    int           __i;                                                      \
    int           __order = 4;                                              \
    int           __has_exclusion = 0;                                      \
    int           __escape;                                                 \
    uint16_t      __cum = 0;                                                \
    uint16_t      __sum = 0;                                                \
    uint32_t      __exclude[8] = {0};                                       \
    if(!__o2(__model)) {                                                    \
        __o2(__model) = allocator_alloc(__model->m_o2_allocator, sizeof(o2_context_t), 1024); \
        __model->m_o2_counts += 1;                                          \
        memset(__o2(__model), 0, sizeof(o2_context_t));                     \
    }                                                                       \
    while(-1) {                                                             \
        __order = 4; __o4_decode(__model, __o4, __coder, __c, do_input, &__escape, &__has_exclusion); if(!__escape) break; \
        __order = 2; __o2_decode(__o2(__model), __coder, __c, do_input, &__escape, &__has_exclusion); if(!__escape) break; \
        __order = 1; __o2_decode(__o1(__model), __coder, __c, do_input, &__escape, &__has_exclusion); if(!__escape) break; \
        __order = 0; __o2_decode(__o0(__model), __coder, __c, do_input, &__escape, &__has_exclusion); if(!__escape) break; \
        for(__i = 0; __i < 256; __i++) { /* decode with o(-1) */            \
            __sum += !__ex_get(__i);                                        \
        }                                                                   \
        rc_decode_cum(__coder, __sum);                                      \
        for(__i = 0; __cum + !__ex_get(__i) <= __coder->m_decode_cum; __i++) { \
            __cum += !__ex_get(__i);                                        \
        }                                                                   \
        rc_decode(__coder, __cum, 1, do_input); __c[0] = __i;               \
        break;                                                              \
    }                                                                       \
    __builtin_prefetch(__model->m_o4_buckets + __o4_hash(model->m_context << 8 | __c[0]), 1, 3); /* prefetch for next round */ \
    switch(__order) { /* fall-through switch */                             \
        case 0: __o2_update(__o0(__model), __c[0]);                         \
        case 1: __o2_update(__o1(__model), __c[0]);                         \
        case 2: __o2_update(__o2(__model), __c[0]);                         \
        case 4: __o4_update(__model, __o4, __c[0]);                         \
    }                                                                       \
    model->m_sse_last_esc = (__order == 4);                                 \
}while(0)

#endif
