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
#ifndef HEADER_SLIMOX_ALLOC_H
#define HEADER_SLIMOX_ALLOC_H

#include <stdlib.h>

#define __allocator_data(blk, nsize, i) ((allocator_element_t*)((blk)->m_data + (nsize)*(i)))

/* memory allocator types */
typedef struct allocator_element_t {
    struct allocator_element_t* m_next;
} allocator_element_t;

typedef struct allocator_block_t {
    struct allocator_block_t* m_next;
    unsigned char m_data[0];
} allocator_block_t;

typedef struct allocator_t {
    struct allocator_block_t*   m_blocklist;
    struct allocator_element_t* m_freelist;
} allocator_t;

static inline allocator_t* allocator_new() {
    allocator_t* allocator = (allocator_t*)malloc(sizeof(allocator_t));
    allocator->m_blocklist = NULL;
    allocator->m_freelist = NULL;
    return allocator;
}

static inline void allocator_del(allocator_t* allocator) {
    allocator_block_t* block;
    while(allocator->m_blocklist) {
        block = allocator->m_blocklist;
        allocator->m_blocklist = allocator->m_blocklist->m_next;
        free(block);
    }
    free(allocator);
    return;
}

static inline void* allocator_alloc(allocator_t* allocator, int nsize, int npoolsize) {
    allocator_block_t*   block;
    allocator_element_t* node;
    int i;

    if(!allocator->m_freelist) { /* freelist is empty, allocate more nodes */
        block = (allocator_block_t*)malloc(sizeof(allocator_block_t) + (nsize * npoolsize));
        block->m_next = allocator->m_blocklist;
        __allocator_data(block, nsize, 0)->m_next = NULL;
        for(i = 1; i < npoolsize; i++) {
            __allocator_data(block, nsize, i)->m_next = __allocator_data(block, nsize, i - 1);
        }
        allocator->m_blocklist = block;
        allocator->m_freelist = __allocator_data(block, nsize, npoolsize - 1);
    }
    node = allocator->m_freelist;
    allocator->m_freelist = allocator->m_freelist->m_next;
    return (void*)node;
}

static inline void allocator_free(allocator_t* allocator, void* node) {
    allocator_element_t* element = (allocator_element_t*)node;
    element->m_next = allocator->m_freelist;
    allocator->m_freelist = element;
    return;
}

static inline int allocator_total_size(allocator_t* allocator, int nsize, int npoolsize) {
    allocator_block_t* block = allocator->m_blocklist;
    int size = 0;

    while(block != NULL) {
        size += sizeof(allocator_block_t) + nsize * npoolsize;
        block = block->m_next;
    }
    return size;
}
#endif
