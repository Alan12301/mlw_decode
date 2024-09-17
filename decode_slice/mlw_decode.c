/*
 * SPDX-FileCopyrightText: Copyright 2020, 2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <math.h>
#include "mlw_common.h"
#include "mlw_decode.h"

#define CHECKED_MALLOC(var, size) { if ( !(var = malloc(size)) ) return decoder->outbuf_size; }

/////////////////////////////// Read from bitstream



// size in byte
static void bitbuf_init( bitbuf_t *bb, uint8_t *buf, int size, int log_symbols) {
    bb->buf  = buf;
    bb->pos  = 0;
    bb->buf_size = size;
    bb->log_symbols = log_symbols;
}

static int bitbuf_getbit( bitbuf_t *bb) {
    int byte_pos = bb->pos>>3;
    int bit_pos = bb->pos&7;
    if ( byte_pos < 0 || byte_pos >= bb->buf_size ) {
        printf("bitbuf_getbit: underrun, bit_pos %3d byte_pos %3d buf_size %3d\n", bit_pos, byte_pos, bb->buf_size);
        exit(1);
    }
    int bit = bb->buf[ byte_pos ] & (1<<bit_pos) ? 1 : 0;
    bb->pos++;
    return bit;
}

static int bitbuf_get( bitbuf_t *bb, const char *name, int len) {
    int i, data=0, save_pos=bb->pos;
    if (len>0) {
        for(i=0; i<len; i++) {
            data |= bitbuf_getbit(bb)<<i;
        }
        if (bb->log_symbols)
            printf("bitbuf: pos %3d %7s len %d data %x\n", save_pos, name, len, data);
    }
    return data;
}

// Decode the given weight stream
//      inbuf       compressed bitstream
//      inbuf_size  size of compressed bitstream in bytes
//      outbuf      uncompressed 9bit signed weights, buffer malloced
//      verbose     if non-zero, printf log
// Return value is the number of uncompressed weights


void mlw_decoder_init(MLWDecoder *decoder, uint8_t *inbuf, int inbuf_size, int16_t **outbuf, int verbose) {
    decoder->nvalues = 0;
    decoder->w_grc_div = 0;
    decoder->w_grc_trunc = 0;
    decoder->w_uncompressed = 0;
    decoder->z_grc_div = 0;
    decoder->z_prev_grc_div = 0;
    decoder->new_palette = 0;
    decoder->palsize = 0;
    decoder->palbits = 0;
    decoder->direct_offset = 0;
    memset(decoder->palette, 0, sizeof(decoder->palette));
    decoder->first = 1;
    decoder->use_zero_run = 0;
    decoder->outbuf_size = 0;
    decoder->nchunks = 0;

    decoder->w_pos=0, decoder->z_pos=0;
    decoder->w_prev_pos=0, decoder->z_prev_pos=0;
    decoder->w_prev_enable=0;
    decoder->w_prev_nsymbols=0;
    memset(decoder->w_prev_q, 0, sizeof(decoder->w_prev_q));
    decoder->z_prev_enable=0;
    decoder->z_prev_nsymbols=0;
    memset(decoder->z_prev_q, 0, sizeof(decoder->z_prev_q));
    decoder->total_zcnt=0;

    decoder->inbuf = inbuf;
    decoder->inbuf_size = inbuf_size;
    decoder->outbuf = outbuf;
    decoder->verbose = verbose;
    *outbuf = 0;

	decoder->bb = &decoder->bitbuf_s;
	 // 初始化 bitbuf
    bitbuf_init(decoder->bb, decoder->inbuf, decoder->inbuf_size, (decoder->verbose & 2) ? 1 : 0);
    decoder->end_decode = 0;
}

int mlw_decode(MLWDecoder *decoder) {
    int *w_value = NULL;
    int *z_value = NULL;

    //
    // Decode slice header
    decoder->z_grc_div = bitbuf_get(decoder->bb, "ZDIV", 3);
    while (decoder->z_grc_div == ZDIV_EOS) {
        bitbuf_get(decoder->bb, "BYTEALIGN", (8 - (decoder->bb->pos & 7)) & 7);
        decoder->first = 1;
        if ((decoder->bb->pos / 8) == decoder->inbuf_size) {
            break;
        }
        decoder->z_grc_div = bitbuf_get(decoder->bb, "ZDIV", 3);
    }
    if ((decoder->bb->pos / 8) == decoder->inbuf_size) {
        decoder->end_decode=1;
        return decoder->outbuf_size;
    }
    assert(decoder->z_grc_div < 4 || decoder->z_grc_div == ZDIV_DISABLE);
    decoder->use_zero_run = decoder->z_grc_div != ZDIV_DISABLE;
    decoder->nvalues = bitbuf_get(decoder->bb, "SLICELEN", 15) + 1;
    decoder->w_grc_div = bitbuf_get(decoder->bb, "WDIV", 3);
    decoder->w_grc_trunc = bitbuf_get(decoder->bb, "WTRUNC", 1);
    decoder->new_palette = bitbuf_get(decoder->bb, "NEWPAL", 1);

    if (decoder->first) {
        assert(decoder->new_palette);
        decoder->first = 0;
    }

    if (!decoder->new_palette) {
        int prev_use_zero_run = decoder->z_prev_grc_div != ZDIV_DISABLE;
        (void)(prev_use_zero_run);
        assert(decoder->use_zero_run == prev_use_zero_run);
    }

    decoder->z_prev_grc_div = decoder->z_grc_div;

    // 設置調色板
    if (decoder->new_palette) {
        decoder->direct_offset = bitbuf_get(decoder->bb, "DIROFS", 5);
        decoder->palsize = bitbuf_get(decoder->bb, "PALSIZE", 5);
        if (decoder->palsize > 0) 
            decoder->palsize++;
        decoder->palbits = bitbuf_get(decoder->bb, "PALBITS", 3) + 2;
        for (int i = 0; i < decoder->palsize; i++) {
            decoder->palette[i] = bitbuf_get(decoder->bb, "PALETTE", decoder->palbits);
        }
    }

    if (decoder->w_grc_div == WDIV_UNCOMPRESSED) {
        decoder->w_uncompressed = 1;
        int uncompressed_bits;
        if (decoder->palsize > 0) {
            uncompressed_bits = 0;
            while ((1 << uncompressed_bits) < decoder->palsize)
                uncompressed_bits++;
        } else {
            uncompressed_bits = decoder->palbits;
        }
        decoder->w_grc_div = uncompressed_bits;
    } else {
        decoder->w_uncompressed = 0;
        assert(decoder->w_grc_div < 6);
    }

    int z_nvalues = decoder->nvalues + (decoder->new_palette ? 1 : 0);
    CHECKED_MALLOC( w_value, decoder->nvalues*sizeof(int) );
    CHECKED_MALLOC( z_value, z_nvalues*sizeof(int) );
    z_value[0] = 0;

    decoder->w_pos=0, decoder->z_pos=0;
    decoder->w_prev_pos=0, decoder->z_prev_pos=0;
    decoder->w_prev_enable=0, decoder->w_prev_nsymbols=0;
	memset(decoder->w_prev_q, 0, sizeof(decoder->w_prev_q));
    decoder->z_prev_enable=0, decoder->z_prev_nsymbols=0;
	memset(decoder->z_prev_q, 0, sizeof(decoder->z_prev_q));
    decoder->total_zcnt=0;
    int w_unary0 = 0, w_unary1 = 0, w_unary1_len = 0, w_q[12] = {0}, w_carry = 0;
    int z_unary = 0, z_q[12] = {0}, z_carry = 0;
    int w_nsymbols = 0;
    int z_nsymbols = 0;
    int z_unary_len = decoder->z_grc_div < 3 ? 12 : 8;

    // Loop over all chunks in the slice
    do {
        int balance = decoder->use_zero_run ? decoder->w_pos - decoder->z_pos : 0;
        int w_enable = (balance < 8 || !decoder->use_zero_run) && decoder->w_pos < decoder->nvalues;
        int z_enable = balance >= 0 && decoder->use_zero_run && decoder->z_pos < z_nvalues;
        //unary0
        if (w_enable) {
            if (!decoder->w_uncompressed)
                w_unary0 = bitbuf_get(decoder->bb, "WUNARY0", 12);
            else
                w_unary0 = 0;
        }
        if (z_enable) {
            z_unary = bitbuf_get(decoder->bb, "ZUNARY", z_unary_len);
            z_nsymbols = 0;
            int cnt = z_carry;
            for (int i = 0; i < z_unary_len; i++) {
                if (z_unary & (1 << i)) {
                    cnt++;
                } else {
                    z_q[z_nsymbols++] = cnt;
                    cnt = 0;                    
                }
            }
            z_carry = cnt;
            decoder->z_pos += z_nsymbols;
        }
        //unary1
        if (w_enable) {
            w_unary1_len = 0;
            int max_symbols = decoder->w_uncompressed && decoder->w_grc_div > 5 ? 8 : 12;
            for (int i = 0; i < max_symbols; i++) {
                if (w_unary0 & (1 << i))
                    w_unary1_len++;
            }
            w_unary1 = bitbuf_get(decoder->bb, "WUNARY1", w_unary1_len);
            w_nsymbols = 0;
            int cnt = w_carry;
            for (int i = 0; i < max_symbols; i++) {
                int code = 0;
                if (w_unary0 & (1 << i)) {
                    code++;
                    if (w_unary1 & 1) {
                        code++;
                    }
                    w_unary1 = w_unary1 >> 1;
                }
                cnt += code;
                if (code < 2 || decoder->w_grc_trunc) {
                    w_q[w_nsymbols++] = cnt;
                    cnt = 0;
                }
            }
            w_carry = cnt;
            decoder->w_pos += w_nsymbols;
        }
        //// value = q * div + remain
        if (decoder->w_prev_enable) {
            for(int i=0; i<decoder->w_prev_nsymbols && decoder->w_prev_pos<decoder->nvalues; i++, decoder->w_prev_pos++) {
                int remain = bitbuf_get( decoder->bb, "WREMAIN", decoder->w_grc_div );//從緩衝區中讀取餘數（WREMAIN）, 長度為 w_grc_div
                w_value[decoder->w_prev_pos] = (decoder->w_prev_q[i]<<decoder->w_grc_div) + remain;//根據解碼得到的數據計算權重值: q * div + remain
		    }
        }
        if (decoder->z_prev_enable) {
            for(int i=0; i<decoder->z_prev_nsymbols && decoder->z_prev_pos<z_nvalues; i++, decoder->z_prev_pos++) {
                int remain = bitbuf_get( decoder->bb, "ZREMAIN", decoder->z_grc_div );
                z_value[decoder->z_prev_pos] = (decoder->z_prev_q[i]<<decoder->z_grc_div) + remain;
                decoder->total_zcnt += z_value[decoder->z_prev_pos];
            }
        }

        decoder->w_prev_enable = w_enable;
        decoder->w_prev_nsymbols = w_nsymbols;
        memcpy(decoder->w_prev_q, w_q, sizeof(decoder->w_prev_q));
        decoder->z_prev_enable = z_enable;
        decoder->z_prev_nsymbols = z_nsymbols;
        memcpy(decoder->z_prev_q, z_q, sizeof(decoder->z_prev_q));
        decoder->nchunks++;
    } while (decoder->w_prev_enable || decoder->z_prev_enable);

    *decoder->outbuf = realloc( *decoder->outbuf, (decoder->outbuf_size + decoder->nvalues + decoder->total_zcnt)*sizeof(int16_t));//擴展output buffer大小
    if (*decoder->outbuf)//成功則
    {
        int k=decoder->outbuf_size;

        // Insert initial zeros緩衝區的開頭插入初始的零值
        // (slices redening the palette may start with zeros)
        if (decoder->new_palette && decoder->use_zero_run) {
            for(int j=0; j<z_value[0]; j++) {
                (*decoder->outbuf)[k++] = 0;
            }
        }
        // Loop over 所有權重並在其間插入零
        // Loop over all weights and insert zeros in-between
        for(int i=0; i<decoder->nvalues; i++) {
            int val;
            // assert(w_value[i]<512); 
            //根據 w_value 的值計算實際的數據 val。
            if (w_value[i]<decoder->palsize) {//palsize預設32，則從palsize中獲取對應的值
                val = decoder->palette[w_value[i]];//Palette Decode
            } else {//Direct Decode
                val = w_value[i]-decoder->palsize+decoder->direct_offset;
            }
            //0~511值域 ==> (-256 ~ 255)
            int sign = val&1;
            int mag  = val>>1;
            (*decoder->outbuf)[k++] = sign ? -mag : mag;
            //zero run
            if (decoder->use_zero_run) {
                for(int j=0; j<z_value[i+(decoder->new_palette?1:0)]; j++) {
                    (*decoder->outbuf)[k++] = 0;
                }
            }
        }

        decoder->outbuf_size = k;
    } else {
        decoder->outbuf_size = 0;
    }
    free(w_value);
    free(z_value);
    w_value = NULL;
    z_value = NULL;

    return decoder->outbuf_size;
}
