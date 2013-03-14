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
#ifndef HEADER_SLIMOX_RC_H
#define HEADER_SLIMOX_RC_H

#include <stdint.h>

#ifndef __param
#define __param(t, p, n) t(__param_##n)=(p); t(n)=(__param_##n)
#endif

/* based on Dmitry Subbotin's carryless range coder */

typedef struct rc_coder_t {
    uint32_t m_low;
    uint32_t m_range;
    uint32_t m_code;
    uint16_t m_decode_cum;
} rc_coder_t;

static const uint32_t rc_top_ = 1 << 24;
static const uint32_t rc_bot_ = 1 << 16;

/*******************************************************************************
 * encode methods
 ******************************************************************************/
#define rc_enc_init(rc)                                                 do{ \
    __param(rc_coder_t*, rc, __rc);                                         \
    __rc->m_low = 0;                                                        \
    __rc->m_range = -1;                                                     \
}while(0)

#define rc_encode(rc, cum, frq, sum, do_output)                         do{ \
    __param(rc_coder_t*, rc, __rc);                                         \
    __param(uint16_t, cum, __cum);                                          \
    __param(uint16_t, frq, __frq);                                          \
    __param(uint16_t, sum, __sum);                                          \
    __rc->m_range /= __sum;                                                 \
    __rc->m_low += __cum * __rc->m_range;                                   \
    __rc->m_range *= __frq;                                                 \
    while((__rc->m_low ^ (__rc->m_low + __rc->m_range)) < rc_top_           \
            || (__rc->m_range < rc_bot_ && ((__rc->m_range = -__rc->m_low & (rc_bot_ - 1)), 1))) { \
        {int output = __rc->m_low >> 24; {do_output;}}                      \
        __rc->m_low <<= 8;                                                  \
        __rc->m_range <<= 8;                                                \
    }                                                                       \
}while(0)

#define rc_enc_flush(rc, do_output)                                     do{ \
    __param(rc_coder_t*, rc, __rc);                                         \
    while(__rc->m_low || (__rc->m_low + __rc->m_range)) {                   \
        {int output = __rc->m_low >> 24; {do_output;}} __rc->m_low <<= 8; __rc->m_range <<= 8; \
    }                                                                       \
}while(0)

/*******************************************************************************
 * decode methods
 ******************************************************************************/
#define rc_dec_init(rc, do_input)                                       do{ \
    __param(rc_coder_t*, rc, __rc);                                         \
    __rc->m_low = 0;                                                        \
    __rc->m_range = -1;                                                     \
    __rc->m_code = 0;                                                       \
    {   int input;                                                          \
        {do_input;} __rc->m_code = (__rc->m_code * 256) + input;            \
        {do_input;} __rc->m_code = (__rc->m_code * 256) + input;            \
        {do_input;} __rc->m_code = (__rc->m_code * 256) + input;            \
        {do_input;} __rc->m_code = (__rc->m_code * 256) + input;            \
    }                                                                       \
}while(0)

#define rc_decode(rc, cum, frq, do_input)                               do{ \
    __param(rc_coder_t*, rc, __rc);                                         \
    __param(uint16_t, cum, __cum);                                          \
    __param(uint16_t, frq, __frq);                                          \
    __rc->m_low += __cum * __rc->m_range;                                   \
    __rc->m_range *= __frq;                                                 \
    while((__rc->m_low ^ (__rc->m_low + __rc->m_range)) < rc_top_           \
            || (__rc->m_range < rc_bot_ && ((__rc->m_range = -__rc->m_low & (rc_bot_ - 1)), 1))) { \
        {   int input;                                                      \
            {do_input;} __rc->m_code = __rc->m_code << 8 | input;           \
        }                                                                   \
        __rc->m_range <<= 8;                                                \
        __rc->m_low <<= 8;                                                  \
    }                                                                       \
}while(0)

#define rc_decode_cum(rc, sum)                                          do{ \
    __param(rc_coder_t*, rc, __rc);                                         \
    __param(uint16_t, sum, __sum);                                          \
    __rc->m_decode_cum = (__rc->m_code - __rc->m_low) / (__rc->m_range /= __sum); \
}while(0)

#endif
