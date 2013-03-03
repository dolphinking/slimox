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

/* o2-structure types */
typedef struct o2_context_t {
    uint8_t  m_sym[256];
    uint8_t  m_frq[256];
    uint16_t m_sum;
    uint16_t m_cnt;
} o2_context_t;

/* o4-structure types */
typedef struct symbol_t {
    struct symbol_t* m_next;
    uint8_t m_sym;
    uint8_t m_frq;
} __attribute__((__packed__)) symbol_t;

typedef struct o4_context_t {
    struct symbol_t*     m_symbols;
    struct o4_context_t* m_next;
    uint32_t m_context;
    uint16_t m_sum;
    uint8_t  m_cnt;
    uint8_t  m_visited;
} __attribute__((__packed__)) o4_context_t;

/* main ppm-model type */
typedef struct ppm_model_t {
    uint32_t m_context;
    uint32_t m_symbol_counts;
    uint32_t m_o2_counts;
    uint32_t m_o4_counts;
    struct o4_context_t* m_o4_buckets[262144];
    struct o2_context_t* m_o2[65536];
    struct o2_context_t  m_o1[256];
    struct o2_context_t  m_o0[1];
    struct allocator_t*  m_o2_allocator;
    struct allocator_t*  m_o4_allocator;
    struct allocator_t*  m_symbol_allocator;
    uint8_t m_sse_ch_context;
    uint8_t m_sse_last_esc;
    struct see_model_t m_binsee[65536];
    struct see_model_t m_see[65536];
    struct see_model_t m_see_01;
    struct see_model_t m_see_10;
} ppm_model_t;

static inline ppm_model_t* ppm_model_new() {
    static const int binsee_init[64][2] = {
        {200, 199}, {200,  59}, {200,  90}, {200, 168}, {200,  22}, {200,  40}, {200,  60}, {200,  70},
        {200,   5}, {200,  15}, {200,  37}, {200,  40}, {200,   4}, {200,   9}, {200,  19}, {200,  22},
        {200,   4}, {200,   5}, {200,  11}, {200,  12}, {200,   3}, {200,   4}, {200,   6}, {200,   6},
        {200,   1}, {200,   2}, {200,   3}, {200,   3}, {200,   1}, {200,   3}, {200,   3}, {200,   3},
        {200, 199}, {200, 152}, {157, 200}, {144, 200}, {200,  51}, {200, 102}, {200, 130}, {200, 116},
        {200,   8}, {200,  19}, {200,  86}, {200,  87}, {200,  11}, {200,  11}, {200,  59}, {200,  58},
        {200,   6}, {200,   9}, {200,  42}, {200,  43}, {200,  10}, {200,   8}, {200,  30}, {200,  32},
        {200,  11}, {200,   7}, {200,  24}, {200,  28}, {200,  28}, {200,  19}, {200,  40}, {200,  28},
    };
    static const int see_init[64][2] = {
        {200, 199}, {200, 115}, {200, 199}, {200, 199}, {200, 199}, {200,  56}, {200,  93}, {200, 199},
        {200, 199}, {200,  29}, {200,  48}, {200,  93}, {200, 199}, {200,  13}, {200,  25}, {200,  46},
        {200, 199}, {200,   6}, {200,  14}, {200,  24}, {200, 199}, {200,   3}, {200,   3}, {200,   8},
        {200, 199}, {200,   2}, {200,   2}, {200,   3}, {200, 199}, {200,  13}, {200,   1}, {200,   2},
        {200, 199}, {200, 171}, {200, 199}, {200, 199}, {200, 199}, {200,  90}, {200, 145}, {200, 199},
        {200, 199}, {200,  51}, {200,  80}, {200, 167}, {200, 199}, {200,  33}, {200,  42}, {200, 102},
        {200, 199}, {200,  20}, {200,  23}, {200,  54}, {200, 199}, {200,  11}, {200,  12}, {200,  23},
        {200, 199}, {200,  12}, {200,   7}, {200,  13}, {200, 199}, {200, 175}, {200,  17}, {200,   9},
    };
    int context_hi;
    int context_lo;

    ppm_model_t* model = (ppm_model_t*)malloc(sizeof(ppm_model_t));
    model->m_context = 0;
    model->m_symbol_counts = 0;
    model->m_o2_counts = 0;
    model->m_o4_counts = 0;
    memset(model->m_o4_buckets, 0, sizeof(model->m_o4_buckets));
    memset(model->m_o2, 0, sizeof(model->m_o2));
    memset(model->m_o1, 0, sizeof(model->m_o1));
    memset(model->m_o0, 0, sizeof(model->m_o0));
    model->m_o4_allocator = allocator_new();
    model->m_symbol_allocator = allocator_new();
    model->m_o2_allocator = allocator_new();
    model->m_sse_ch_context = 0;
    for(context_hi = 0; context_hi < 64; context_hi++) {
        for(context_lo = 0; context_lo < 1024; context_lo++) {
            model->m_binsee[context_hi << 10 | context_lo].m_c[0] = binsee_init[context_hi][0];
            model->m_binsee[context_hi << 10 | context_lo].m_c[1] = binsee_init[context_hi][1];
            model->m_see[context_hi << 10 | context_lo].m_c[0] = see_init[context_hi][0];
            model->m_see[context_hi << 10 | context_lo].m_c[1] = see_init[context_hi][1];
        }
    }
    model->m_see_01.m_c[0] = 0;
    model->m_see_01.m_c[1] = 1;
    model->m_see_10.m_c[0] = 1;
    model->m_see_10.m_c[1] = 0;
    return model;
};

static inline void ppm_model_del(ppm_model_t* model) {
    allocator_del(model->m_o4_allocator);
    allocator_del(model->m_symbol_allocator);
    allocator_del(model->m_o2_allocator);
    free(model);
    return;
}

#define __o4_hash(model) ((unsigned)((model)->m_context ^ (model)->m_context >> 14) % 262144)
#define __o2(model)      ( (model)->m_o2[(model)->m_context & 0xffff])
#define __o1(model)      (&(model)->m_o1[(model)->m_context & 0x00ff])
#define __o0(model)      (&(model)->m_o0[0])
#define __ex_get(n)      ((__exclude[(n) / 32] >> ((n) % 32)) & 0x01)
#define __ex_set(n)      ((__exclude[(n) / 32] |= 0x01<< ((n) % 32)))

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
    int curcnt = o4->m_cnt + (256 & -(o4->m_cnt == 0 && o4->m_symbols != NULL));
    int lowsum = __o2(model)->m_sum;
    int lowcnt = __o2(model)->m_cnt;
    uint32_t context = 0;

    if(curcnt == 256) return &model->m_see_10; /* all symbols appeared -- never esxape */
    if(curcnt == 0)   return &model->m_see_01; /* no symbols nnder current context -- always escape */
    if(curcnt == 1) {
        /* QUANTIZE(binary) = (last_esc[1] | sum[3] | lowcnt[2] | lowsum[1] | bin_symbol[3] | previous symbols[6]) */
        context |= ((model->m_context >>  6) & 0x03);
        context |= ((model->m_context >> 14) & 0x03) << 2;
        context |= ((model->m_context >> 22) & 0x03) << 4;
        context |= (o4->m_symbols->m_sym >> 6) << 6;
        context |= (lowsum >= 4) << 9;
        context |= min2(__fast_log2(lowcnt / 2), 3) << 10;
        context |= min2(__fast_log2(o4->m_sum / 3), 7) << 12;
        context |= model->m_sse_last_esc << 15;
        return &model->m_binsee[context];
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
        return &model->m_see[context];
    }
    return NULL;
}

#define __o2_encode(o2, coder, c, do_output, escape)                    do{ \
    __param(o2_context_t*, o2, __o2);                                       \
    __param(rc_coder_t*, coder, __coder);                                   \
    __param(unsigned char, c, __c);                                         \
    __param(int*, escape, __escape);                                        \
    int __found = 0;                                                        \
    int __found_index = 0;                                                  \
    int __i;                                                                \
    uint16_t __cumx = 0;                                                    \
    uint16_t __frqx = 0;                                                    \
    uint16_t __sumx = 0;                                                    \
    uint16_t __escx = 0;                                                    \
    uint16_t __recent_frq = __o2->m_frq[0] & -!__ex_get(__o2->m_sym[0]);    \
    for(__i = 0; __i < __o2->m_cnt; __i++) {                                \
        __cumx += __o2->m_frq[__i] & -(!__ex_get(__o2->m_sym[__i]) && !__found && __o2->m_sym[__i] != __c); \
        __sumx += __o2->m_frq[__i] & -(!__ex_get(__o2->m_sym[__i]));        \
        __escx += __o2->m_frq[__i] == 1;                                    \
        if(__o2->m_sym[__i] == __c) {                                       \
            __found_index = __i;                                            \
            __found = 1;                                                    \
            swap(__o2->m_frq[__i], __o2->m_frq[0]);                         \
            swap(__o2->m_sym[__i], __o2->m_sym[0]);                         \
        }                                                                   \
    }                                                                       \
    __frqx = __o2->m_frq[0] + (__recent_frq & -(__found_index == 0));       \
    __cumx += __recent_frq & -(__found_index > 0);                          \
    __escx += !__escx;                                                      \
    __sumx += __recent_frq + __escx;                                        \
    if(!__found) {                                                          \
        __escape[0] = 1;                                                    \
        for(__i = 0; __i < __o2->m_cnt; __i++) { /* exclude */              \
            if(__o2->m_frq[__i] != 0) {                                     \
                __ex_set(__o2->m_sym[__i]);                                 \
            }                                                               \
        }                                                                   \
        __o2->m_frq[__o2->m_cnt] = __o2->m_frq[0];                          \
        __o2->m_sym[__o2->m_cnt] = __o2->m_sym[0];                          \
        __o2->m_sym[0] = __c;                                               \
        __o2->m_frq[0] = 0;                                                 \
        __o2->m_cnt += 1;                                                   \
        __cumx = __sumx - __escx;                                           \
        __frqx = __escx;                                                    \
    } else {                                                                \
        __escape[0] = 0;                                                    \
    }                                                                       \
    rc_encode(__coder, __cumx, __frqx, __sumx, do_output);                  \
}while(0)

#define __o2_decode(o2, coder, c, do_input, escape)                     do{ \
    __param(o2_context_t*, o2, __o2);                                       \
    __param(rc_coder_t*, coder, __coder);                                   \
    __param(unsigned char*, c, __c);                                        \
    __param(int*, escape, __escape);                                        \
    int __i;                                                                \
    uint16_t __cumx = 0;                                                    \
    uint16_t __frqx = 0;                                                    \
    uint16_t __sumx = 0;                                                    \
    uint16_t __escx = 0;                                                    \
    uint16_t __recent_frq = __o2->m_frq[0] & -!__ex_get(__o2->m_sym[0]);    \
    for(__i = 0; __i < __o2->m_cnt; __i++) {                                \
        __sumx += __o2->m_frq[__i] & -!__ex_get(__o2->m_sym[__i]);          \
        __escx += __o2->m_frq[__i] == 1;                                    \
    }                                                                       \
    __escx += !__escx;                                                      \
    __sumx += __recent_frq + __escx;                                        \
    rc_decode_cum(__coder, __sumx);                                         \
    if(__sumx - __escx <= __coder->m_decode_cum) {                          \
        __escape[0] = 1;                                                    \
        for(__i = 0; __i < __o2->m_cnt; __i++) { /* exclude */              \
            __ex_set(__o2->m_sym[__i]);                                     \
        }                                                                   \
        __o2->m_frq[__o2->m_cnt] = __o2->m_frq[0];                          \
        __o2->m_sym[__o2->m_cnt] = __o2->m_sym[0];                          \
        __o2->m_frq[0] = 0;                                                 \
        __o2->m_cnt += 1;                                                   \
        __cumx = __sumx - __escx;                                           \
        __frqx = __escx;                                                    \
    } else {                                                                \
        __escape[0] = 0;                                                    \
        __i = 0;                                                            \
        while(__cumx + __recent_frq + (__o2->m_frq[__i] & -!__ex_get(__o2->m_sym[__i])) <= __coder->m_decode_cum) { \
            __cumx += __o2->m_frq[__i] & -!__ex_get(__o2->m_sym[__i]);      \
            __i++;                                                          \
        }                                                                   \
        __frqx = __o2->m_frq[__i];                                          \
        __c[0] = __o2->m_sym[__i];                                          \
        if(__i == 0) {                                                      \
            __frqx += __recent_frq;                                         \
        } else {                                                            \
            swap(__o2->m_frq[__i], __o2->m_frq[0]);                         \
            swap(__o2->m_sym[__i], __o2->m_sym[0]);                         \
            __cumx += __recent_frq;                                         \
        }                                                                   \
    }                                                                       \
    rc_decode(__coder, __cumx, __frqx, do_input);                           \
}while(0)

static inline void __o2_update(o2_context_t* o2, int c) {
    int n = 0;
    int i;

    o2->m_sum += 1;
    o2->m_frq[0] += 1;
    o2->m_sym[0] = c;

    if(o2->m_frq[0] > 250) { /* rescale */
        o2->m_cnt = 0;
        o2->m_sum = 0;
        for(i = 0; i + n < 256; i++) {
            if((o2->m_frq[i] = o2->m_frq[i + n] / 2) > 0) {
                o2->m_sym[i] = o2->m_sym[i + n];
                o2->m_cnt += 1;
                o2->m_sum += o2->m_frq[i];
            } else {
                n++;
                i--;
            }
        }
        memset(o2->m_sym + o2->m_cnt, 0, sizeof(o2->m_sym[0]) * (256 - o2->m_cnt));
        memset(o2->m_frq + o2->m_cnt, 0, sizeof(o2->m_frq[0]) * (256 - o2->m_cnt));
    }
    return;
}

static inline o4_context_t* __o4_context_node(ppm_model_t* model) {
    o4_context_t* o4;
    o4_context_t* o4_prev;
    symbol_t*     symbol;
    int i;

    if(model->m_o4_counts >= 1048576 || model->m_symbol_counts >= 16777216) { /* too many o4-context/symbol nodes */
        model->m_symbol_counts = 0;
        model->m_o2_counts = 0;
        for(i = 0; i < 262144; i++) {
            while(model->m_o4_buckets[i] && model->m_o4_buckets[i]->m_visited / 2 == 0) {
                o4 = model->m_o4_buckets[i];
                model->m_o4_buckets[i] = model->m_o4_buckets[i]->m_next;
                while(o4->m_symbols != NULL) { /* free symbols */
                    symbol = o4->m_symbols;
                    o4->m_symbols = o4->m_symbols->m_next;
                    allocator_free(model->m_symbol_allocator, symbol);
                }
                allocator_free(model->m_o4_allocator, o4);
                model->m_o4_counts -= 1;
            }
            for(o4 = model->m_o4_buckets[i]; o4 != NULL; o4 = o4->m_next) {
                if((o4->m_visited /= 2) == 0) {
                    o4_prev->m_next = o4->m_next;
                    while(o4->m_symbols != NULL) { /* free symbols */
                        symbol = o4->m_symbols;
                        o4->m_symbols = o4->m_symbols->m_next;
                        allocator_free(model->m_symbol_allocator, symbol);
                    }
                    allocator_free(model->m_o4_allocator, o4);
                    model->m_o4_counts -= 1;
                    o4 = o4_prev;
                } else {
                    model->m_o4_counts += 1;
                    model->m_symbol_counts += o4->m_cnt;
                    o4_prev = o4;
                }
            }
        }
    }

    o4 = model->m_o4_buckets[__o4_hash(model)];
    o4_prev = NULL;
    while(o4 && o4->m_context != model->m_context) { /* search for o4 context */
        o4_prev = o4;
        o4 = o4->m_next;
    }
    if(o4 != NULL) { /* found -- bring to front of linked-list */
        if(o4_prev != NULL) {
            o4_prev->m_next = o4->m_next;
            o4->m_next = model->m_o4_buckets[__o4_hash(model)];
            model->m_o4_buckets[__o4_hash(model)] = o4;
        }
    } else { /* not found -- create new node for context */
        o4 = allocator_alloc(model->m_o4_allocator, sizeof(o4_context_t) + sizeof(symbol_t), 8192);
        allocator_free(model->m_symbol_allocator, o4 + 1);
        o4->m_context = model->m_context;
        o4->m_symbols = NULL;
        o4->m_sum = 0;
        o4->m_cnt = 0;
        o4->m_visited = 0;
        o4->m_next = model->m_o4_buckets[__o4_hash(model)];
        model->m_o4_counts += 1;
        model->m_o4_buckets[__o4_hash(model)] = o4;
    }
    o4->m_visited += (o4->m_visited < 255);
    return o4;
}

#define __o4_encode(model, o4, coder, c, do_output, escape)             do{ \
    __param(ppm_model_t*, model, __model);                                  \
    __param(rc_coder_t*, coder, __coder);                                   \
    __param(o4_context_t*, o4, __o4);                                       \
    __param(uint8_t, c, __c);                                               \
    __param(int*, escape, __escape);                                        \
    symbol_t* __o4_sym_node = __o4->m_symbols;                              \
    symbol_t* __o4_sym_prev = NULL;                                         \
    see_model_t* __see = __ppm_see_context_o4(__model, __o4);               \
    uint16_t     __cum = 0;                                                 \
    uint16_t     __frq = 0;                                                 \
    uint16_t     __recent_frq = 0;                                          \
    while(__o4_sym_node != NULL && __o4_sym_node->m_sym != __c) { /* search for symbol */ \
        __cum += __o4_sym_node->m_frq;                                      \
        __o4_sym_prev = __o4_sym_node;                                      \
        __o4_sym_node = __o4_sym_node->m_next;                              \
    }                                                                       \
    if(__o4_sym_node != NULL) { /* found -- bring to front of linked-list */\
        rc_encode(__coder, 0, __see->m_c[0], __see->m_c[0] + __see->m_c[1], do_output); \
        __ppm_see_update(__see, 0, 35);                                     \
        __escape[0] = 0;                                                    \
        if(__o4->m_cnt != 1) { /* no need to encode binary context */       \
            __recent_frq = (__o4->m_symbols->m_frq + 6) / 2; /* recency scaling */ \
            if(__o4_sym_node == __o4->m_symbols) {                          \
                __frq = __o4_sym_node->m_frq + __recent_frq;                \
            } else {                                                        \
                __frq = __o4_sym_node->m_frq;                               \
                __cum += __recent_frq;                                      \
                __o4_sym_prev->m_next = __o4_sym_node->m_next;              \
                __o4_sym_node->m_next = __o4->m_symbols;                    \
                __o4->m_symbols = __o4_sym_node;                            \
            }                                                               \
            rc_encode(__coder, __cum, __frq, __o4->m_sum + __recent_frq, do_output); \
        }                                                                   \
    } else { /* not found -- create new node for sym */                     \
        rc_encode(__coder, __see->m_c[0], __see->m_c[1], __see->m_c[0] + __see->m_c[1], do_output); \
        __ppm_see_update(__see, 1, 35);                                     \
        __escape[0] = 1;                                                    \
        for(__o4_sym_prev = __o4->m_symbols; __o4_sym_prev != NULL; __o4_sym_prev = __o4_sym_prev->m_next) { \
            __ex_set(__o4_sym_prev->m_sym); /* exclude o4 */                \
        }                                                                   \
        __o4_sym_node = allocator_alloc(__model->m_symbol_allocator, sizeof(symbol_t), 8192); \
        __o4_sym_node->m_sym = __c;                                         \
        __o4_sym_node->m_frq = 0;                                           \
        __o4_sym_node->m_next = __o4->m_symbols;                            \
        __o4->m_symbols = __o4_sym_node;                                    \
        __o4->m_cnt += 1;                                                   \
        __model->m_symbol_counts += 1;                                      \
    }                                                                       \
}while(0)

#define __o4_decode(model, o4, coder, c, do_input, escape)              do{ \
    __param(ppm_model_t*, model, __model);                                  \
    __param(rc_coder_t*, coder, __coder);                                   \
    __param(o4_context_t*, o4, __o4);                                       \
    __param(uint8_t*, c, __c);                                              \
    __param(int*, escape, __escape);                                        \
    symbol_t* __o4_sym_node = __o4->m_symbols;                              \
    symbol_t* __o4_sym_prev = NULL;                                         \
    see_model_t* __see = __ppm_see_context_o4(__model, __o4);               \
    uint16_t     __cum = 0;                                                 \
    uint16_t     __frq = 0;                                                 \
    uint16_t     __recent_frq = 0;                                          \
    rc_decode_cum(__coder, __see->m_c[0] + __see->m_c[1]);                  \
    if(__see->m_c[0] <= __coder->m_decode_cum) {                            \
        rc_decode(__coder, __see->m_c[0], __see->m_c[1], do_input);         \
        __ppm_see_update(__see, 1, 35);                                     \
        __escape[0] = 1;                                                    \
        for(__o4_sym_prev = __o4->m_symbols; __o4_sym_prev != NULL; __o4_sym_prev = __o4_sym_prev->m_next) { \
            __ex_set(__o4_sym_prev->m_sym); /* exclude o4 */                \
        }                                                                   \
        __o4_sym_node = allocator_alloc(__model->m_symbol_allocator, sizeof(symbol_t), 8192); \
        __o4_sym_node->m_sym = 0; /* sym is unknown at the moment*/         \
        __o4_sym_node->m_frq = 0;                                           \
        __o4_sym_node->m_next = __o4->m_symbols;                            \
        __o4->m_symbols = __o4_sym_node;                                    \
        __o4->m_cnt += 1;                                                   \
        __model->m_symbol_counts += 1;                                      \
    } else { /* found */                                                    \
        rc_decode(__coder, 0, __see->m_c[0], do_input);                     \
        __ppm_see_update(__see, 0, 35);                                     \
        __escape[0] = 0;                                                    \
        if(__o4->m_cnt != 1) { /* no need to decode binary context */       \
            __recent_frq = (__o4->m_symbols->m_frq + 6) / 2; /* recency scaling */ \
            rc_decode_cum(__coder, __o4->m_sum + __recent_frq);             \
            while(__cum + __recent_frq + __o4_sym_node->m_frq <= __coder->m_decode_cum) { \
                __cum += __o4_sym_node->m_frq;                              \
                __o4_sym_prev = __o4_sym_node;                              \
                __o4_sym_node = __o4_sym_node->m_next;                      \
            }                                                               \
            if(__o4_sym_node == __o4->m_symbols) {                          \
                __frq = __o4_sym_node->m_frq + __recent_frq;                \
            } else {                                                        \
                __frq = __o4_sym_node->m_frq;                               \
                __cum += __recent_frq;                                      \
                __o4_sym_prev->m_next = __o4_sym_node->m_next;              \
                __o4_sym_node->m_next = __o4->m_symbols;                    \
                __o4->m_symbols = __o4_sym_node;                            \
            }                                                               \
            rc_decode(__coder, __cum, __frq, do_input);                     \
        }                                                                   \
        __c[0] = __o4->m_symbols->m_sym;                                    \
    }                                                                       \
}while(0)

static inline void __o4_update(ppm_model_t* model, o4_context_t* o4, int c) {
    symbol_t* o4_sym_prev = (symbol_t*)o4; /* first member is the same "->m_next" */
    symbol_t* o4_sym_node;
    o2_context_t* o2 = __o2(model);
    int i;
    int o2cnt = o2->m_cnt;
    int o2sum = o2->m_sum;
    int o2frq = 0;

    o4->m_symbols->m_sym = c;
    if(o4->m_symbols->m_frq == 0) { /* calculate init frequency */
        for(i = 0; i < o2cnt; i++) {
            if(o2->m_sym[i] == c) {
                o2frq = o2->m_frq[i];
                break;
            }
        }
        if(o4->m_sum == 0) {
            o4->m_symbols->m_frq = 2 + o2frq * 2 / (o2sum + o2cnt + 1 - o2frq);
        } else {
            o4->m_symbols->m_frq = 2 + o2frq * (o4->m_sum + o4->m_cnt + 1) / (o2sum + o2cnt + o4->m_sum - o2frq);
        }
        o4->m_symbols->m_frq = (o4->m_symbols->m_frq > 0) ? o4->m_symbols->m_frq : 1;
        o4->m_symbols->m_frq = (o4->m_symbols->m_frq < 8) ? o4->m_symbols->m_frq : 7;
        o4->m_sum += o4->m_symbols->m_frq;
    } else {
        /* update freq */
        o4->m_sum += 3;
        o4->m_symbols->m_frq += 3;
    }

    if(o4->m_symbols->m_frq > 250 || o4->m_sum > 32000) { /* rescale */
        o4_sym_node = o4->m_symbols;
        model->m_symbol_counts -= o4->m_cnt;
        o4->m_sum = 0;
        o4->m_cnt = 0;
        while(o4_sym_node != NULL) { /* halves frequency of all symbols */
            if((o4_sym_node->m_frq /= 2) == 0) { /* remove nodes with zero frequency */
                o4_sym_prev->m_next = o4_sym_node->m_next;
                allocator_free(model->m_symbol_allocator, o4_sym_node);
                o4_sym_node = o4_sym_prev->m_next;
            } else {
                o4->m_sum += o4_sym_node->m_frq;
                o4->m_cnt += 1;
                o4_sym_prev = o4_sym_node;
                o4_sym_node = o4_sym_node->m_next;
                model->m_symbol_counts += 1;
            }
        }
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
    int           __order = 4;                                              \
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
        __order = 4; __o4_encode(__model, __o4, __coder, __c, do_output, &__escape); if(!__escape) break; \
        __order = 2; __o2_encode(__o2(__model), __coder, __c, do_output, &__escape); if(!__escape) break; \
        __order = 1; __o2_encode(__o1(__model), __coder, __c, do_output, &__escape); if(!__escape) break; \
        __order = 0; __o2_encode(__o0(__model), __coder, __c, do_output, &__escape); if(!__escape) break; \
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
        __order = 4; __o4_decode(__model, __o4, __coder, __c, do_input, &__escape); if(!__escape) break; \
        __order = 2; __o2_decode(__o2(__model), __coder, __c, do_input, &__escape); if(!__escape) break; \
        __order = 1; __o2_decode(__o1(__model), __coder, __c, do_input, &__escape); if(!__escape) break; \
        __order = 0; __o2_decode(__o0(__model), __coder, __c, do_input, &__escape); if(!__escape) break; \
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
    switch(__order) { /* fall-through switch */                             \
        case 0: __o2_update(__o0(__model), __c[0]);                         \
        case 1: __o2_update(__o1(__model), __c[0]);                         \
        case 2: __o2_update(__o2(__model), __c[0]);                         \
        case 4: __o4_update(__model, __o4, __c[0]);                         \
    }                                                                       \
    model->m_sse_last_esc = (__order == 4);                                 \
}while(0)

#endif
