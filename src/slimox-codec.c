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
#include "slimox-ppm.h"
#include "slimox-buf.h"
#include "slimox-match.h"

int slimox_encode(buf_t* ib, buf_t* ob, ppm_model_t* ppm, FILE* fout_sync) {
    int outsize = 0;
    int pos = 0;
    int match_len;
    int i;
    int counts[256] = {0};
    int escape = 0;

    matcher_t matcher;
    rc_coder_t coder;

    /* count for escape char */
    for(i = 0; i < ib->m_size; i++) {
        counts[ib->m_data[i]] += 1;
    }
    for(i = 1; i < 256; i++) {
        escape = (counts[escape] > counts[i]) ? i : escape;
    }

    /* write output size */
    buf_append(ob, ib->m_size / 16777216 % 256);
    buf_append(ob, ib->m_size / 65536 % 256);
    buf_append(ob, ib->m_size / 256 % 256);
    buf_append(ob, ib->m_size / 1 % 256);
    buf_append(ob, escape);

    /* start encoding */
    matcher_init(&matcher);
    rc_enc_init(&coder);
    while(pos < ib->m_size) {
        match_len = 1;
        if(pos > 8 && pos < ib->m_size - 256) { /* avoid overflow */
            match_len = matcher_lookup(&matcher, ib->m_data, pos);
        }

        if(match_len >= match_min) { /* encode a match */
            ppm_encode(ppm, &coder, escape, ;; buf_append(ob, output));
            ppm->m_context <<= 8;
            ppm->m_context |= escape;
            ppm_encode(ppm, &coder, match_len, ;; buf_append(ob, output));

        } else { /* encode a literal */
            ppm_encode(ppm, &coder, ib->m_data[pos], ;; buf_append(ob, output));
            if(ib->m_data[pos] == escape) {
                ppm->m_context <<= 8;
                ppm->m_context |= escape;
                ppm_encode(ppm, &coder, 0, ;; buf_append(ob, output));
            }
        }
        for(i = 0; i < match_len; i++) { /* update context */
            matcher_update(&matcher, ib->m_data, pos, 1);
            ppm->m_context <<= 8;
            ppm->m_context |= ib->m_data[pos];
            pos++;
        }

        if(fout_sync && ob->m_size > 16384) {
            if(fout_sync) {
                fwrite(ob->m_data, 1, ob->m_size, fout_sync); /* sync to file */
                outsize += ob->m_size;
                ob->m_size = 0;
            }
            fprintf(stderr, "%d => %d\r", pos, outsize);
        }
    }
    matcher_free(&matcher);
    rc_enc_flush(&coder, buf_append(ob, output));

    if(fout_sync) { /* sync last output bytes to file */
        fwrite(ob->m_data, 1, ob->m_size, fout_sync);
        outsize += ob->m_size;
        ob->m_size = 0;
    }
    fprintf(stderr, "%d => %d\n", pos, outsize);
    return outsize;
}

int slimox_decode(buf_t* ib, buf_t* ob, ppm_model_t* ppm) {
    uint32_t   size = 0;
    uint32_t   match_pos = 0;
    uint8_t    match_len = 0;
    uint8_t    c;
    int        percent = 0;
    int        pos = 0;
    int        escape;
    int        i;
    matcher_t  matcher;
    rc_coder_t coder;

    /* read output size */
    size = size * 256 + ib->m_data[pos++];
    size = size * 256 + ib->m_data[pos++];
    size = size * 256 + ib->m_data[pos++];
    size = size * 256 + ib->m_data[pos++];
    escape = ib->m_data[pos++];

    /* start decoding */
    matcher_init(&matcher);
    rc_dec_init(&coder, input = ib->m_data[pos++]);
    while(ob->m_size < size) {
        if(percent < pos / (ib->m_size / 100 + 1)) {
            percent = pos / (ib->m_size / 100 + 1);
            fprintf(stderr, "%d <= %d\r", ob->m_size, pos);
        }

        match_len = 1;
        ppm_decode(ppm, &coder, &c, ;; input = ib->m_data[pos++]);

        if(c != escape) { /* literal */
            buf_append(ob, c);
        } else {
            ppm->m_context <<= 8;
            ppm->m_context |= c;
            ppm_decode(ppm, &coder, &match_len, ;; input = ib->m_data[pos++]);
            if(match_len == 0) { /* escape */
                match_len = 1;
                buf_append(ob, escape);
            } else { /* match */
                match_pos = matcher_getpos(&matcher, ob->m_data, ob->m_size);
                for(i = 0; i < match_len; i++) {
                    buf_append(ob, ob->m_data[match_pos + i]);
                }
            }
        }
        while(match_len > 0) { /* update context */
            matcher_update(&matcher, ob->m_data, ob->m_size - match_len, 0);
            ppm->m_context <<= 8;
            ppm->m_context |= ob->m_data[ob->m_size - match_len];
            match_len--;
        }
    }
    fprintf(stderr, "%d <= %d\n", ob->m_size, pos);
    matcher_free(&matcher);
    return 0;
}
