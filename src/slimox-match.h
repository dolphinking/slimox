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
#ifndef HEADER_SLIMOX_MATCH_H
#define HEADER_SLIMOX_MATCH_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static const int match_min = 8;
static const int match_max = 255;

typedef struct matcher_t {
    uint64_t* m_lzp8;
    uint64_t* m_lzp4;
    uint32_t* m_lzp2;
} matcher_t;

#define M_hash2_(x)     ((*(uint16_t*)(x)))
#define M_hash4_(x)     ((*(uint32_t*)(x) ^ (*(uint32_t*)(x) >>  6) ^ (*(uint32_t*)(x) >> 12)) & 0xfffff)
#define M_hash8_(x)     ((*(uint64_t*)(x) ^ (*(uint64_t*)(x) >> 22) ^ (*(uint64_t*)(x) >> 44)) & 0xfffff)

static inline void matcher_init(matcher_t* matcher) {
    matcher->m_lzp8 = calloc((1 << 20), sizeof(uint64_t));
    matcher->m_lzp4 = calloc((1 << 20), sizeof(uint64_t));
    matcher->m_lzp2 = calloc((1 << 16), sizeof(uint32_t));
    return;
}

static inline void matcher_free(matcher_t* matcher) {
    free(matcher->m_lzp8);
    free(matcher->m_lzp4);
    free(matcher->m_lzp2);
    return;
}

static inline uint32_t matcher_getpos(matcher_t* matcher, unsigned char* data, uint32_t pos) {
    uint32_t context4 = *(uint32_t*)(data + pos - 4);
    uint32_t lzpos[3] = {
        matcher->m_lzp8[M_hash8_(data + pos - 8)] & 0xffffffff,
        matcher->m_lzp4[M_hash4_(data + pos - 4)] & 0xffffffff,
        matcher->m_lzp2[M_hash2_(data + pos - 2)],
    };
    if(matcher->m_lzp8[M_hash8_(data + pos - 8)] >> 32 == context4 && lzpos[0] != 0) return lzpos[0];
    if(matcher->m_lzp4[M_hash4_(data + pos - 4)] >> 32 == context4 && lzpos[1] != 0) return lzpos[1];
    return lzpos[2];
}

static inline uint32_t matcher_lookup(matcher_t* matcher, unsigned char* data, uint32_t pos) {
    uint32_t match_pos = matcher_getpos(matcher, data, pos);
    uint32_t match_len = 0;

    /* match content */
    while(match_pos != 0 && match_len < match_max && data[match_pos + match_len] == data[pos + match_len]) {
        match_len++;
    }
    return (match_len >= match_min) ? match_len : 1;
}

static inline void matcher_update(matcher_t* matcher, unsigned char* data, uint32_t pos, int encode) {
    uint32_t context4;

    if(pos >= 12) { /* avoid overflow */
        context4 = *(uint32_t*)(data + pos - 4);

        matcher->m_lzp8[M_hash8_(data + pos - 8)] = pos | (uint64_t)context4 << 32;
        matcher->m_lzp4[M_hash4_(data + pos - 4)] = pos | (uint64_t)context4 << 32;
        matcher->m_lzp2[M_hash2_(data + pos - 2)] = pos;

        /* prefetch for next round */
        if(encode) {
            __builtin_prefetch(matcher->m_lzp8 + M_hash8_(data + pos - 6), 1, 3);
            __builtin_prefetch(matcher->m_lzp4 + M_hash4_(data + pos - 2), 1, 3);
            __builtin_prefetch(matcher->m_lzp2 + M_hash2_(data + pos + 0), 1, 3);
        } else {
            __builtin_prefetch(matcher->m_lzp8 + M_hash8_(data + pos - 7), 1, 3);
            __builtin_prefetch(matcher->m_lzp4 + M_hash4_(data + pos - 3), 1, 3);
            __builtin_prefetch(matcher->m_lzp2 + M_hash2_(data + pos - 1), 1, 3);
        }
    }
    return;
}
#endif
