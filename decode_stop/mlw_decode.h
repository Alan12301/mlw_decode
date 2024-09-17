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

#include <stdint.h>

#ifndef MLW_DECODE_H
#define MLW_DECODE_H

#ifdef _MSC_VER
  #define MLW_DECODE_EXPORTED __declspec(dllexport)
#else
  #define MLW_DECODE_EXPORTED __attribute__((visibility("default")))
#endif

#if __cplusplus
extern "C"
{
#endif
typedef struct bitbuf {
    uint8_t *buf;
    int buf_size;               // in bytes
    int pos;                    // bit pos of next bit
    int log_symbols;
} bitbuf_t;

typedef struct {
	int nvalues;    
	int w_grc_div;  
	int w_grc_trunc;
	int w_uncompressed; 
	int z_grc_div, z_prev_grc_div;
	int new_palette;
	int palsize, palbits;   
	int direct_offset;
	int16_t palette[512];
	int first;  
	int use_zero_run;   
	int outbuf_size;
	int nchunks;
	//
	int w_pos, z_pos;
	int w_prev_pos, z_prev_pos;
	int w_prev_enable, w_prev_nsymbols, w_prev_q[12];
	int z_prev_enable, z_prev_nsymbols, z_prev_q[12];
	int total_zcnt;
	//for 256
	int w_carry;
	int state;
	int last;
	int w_enable_temp;
	int w_q_temp[12];
	int w_nsymbols_temp;

	uint8_t *inbuf;
	int inbuf_size;
	int16_t **outbuf;
	int verbose;

	bitbuf_t bitbuf_s;
	bitbuf_t *bb ;
	int end_decode;
} MLWDecoder;

MLW_DECODE_EXPORTED
void mlw_decoder_init(MLWDecoder *decoder, uint8_t *inbuf, int inbuf_size, int16_t **outbuf, int verbose);
MLW_DECODE_EXPORTED
int mlw_decode(MLWDecoder *decoder, int stop);

#if __cplusplus
}
#endif

#endif
