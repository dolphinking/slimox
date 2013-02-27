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
#ifndef HEADER_SLIMOX_BUF_H
#define HEADER_SLIMOX_BUF_H

#include <stdlib.h>

typedef struct buf_t {
    uint8_t* m_data;
    uint32_t m_size;
    uint32_t m_capacity;
} buf_t;

static inline buf_t* buf_new() {
    buf_t* buf = malloc(sizeof(buf_t));
    buf->m_data = NULL;
    buf->m_size = 0;
    buf->m_capacity = 0;
    return buf;
}
static inline void buf_del(buf_t* buf) {
    free(buf->m_data);
    free(buf);
    return;
}

static inline void buf_resize(buf_t* buf, uint32_t size) {
    if(size > buf->m_capacity || size < buf->m_capacity / 2) {
        buf->m_capacity = size * 1.2;
        buf->m_data = realloc(buf->m_data, buf->m_capacity);
    }
    buf->m_size = size;
    return;
}

static inline void buf_append(buf_t* buf, uint8_t byte) {
    if(buf->m_size == buf->m_capacity) {
        buf->m_capacity = buf->m_size * 1.2 + 1;
        buf->m_data = realloc(buf->m_data, buf->m_capacity);
    }
    buf->m_data[buf->m_size++] = byte;
    return;
}
#endif
