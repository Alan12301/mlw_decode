# slice decode
* 每次處理一個slice
* 在main.c內使用迴圈運行
* 有新設置的參數:int end_decode ... 
```
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
	uint8_t *inbuf;
	int inbuf_size;
	int16_t **outbuf;
	int verbose;

	bitbuf_t bitbuf_s;
	bitbuf_t *bb ;
	int end_decode;
} MLWDecoder;
```
* decoder 初始化
```
void mlw_decoder_init(MLWDecoder *decoder, uint8_t *inbuf, int inbuf_size, int16_t **outbuf, int verbose)
```
* decode function
```
int mlw_decode(MLWDecoder *decoder)
```
* 結果:
![image](https://hackmd.io/_uploads/HkCm8ohhC.png)

# 
