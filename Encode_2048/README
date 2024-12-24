# 分析學長給的mlw_codec:
## "Get weights" (or weight indicies) AND zero-runs from the input weight stream.
* 計算tile_size = tile_i_size * tile_o_size;
    * tile_i_size
    * tile_o_size
* 根據有沒有使用zero_run決定**weight_values**的值
* 记录当前 Tile 的权重值和零运行长度结束位置。递增 Tile 计数器 tile_num

## Search for good "GRC parameters" for the "weight" stream
* 为每个 Tile 计算最佳的（GRC 参数）并统计编码所需的总位数（total_w_bitcnt）
    * 先確認每個tile的w_start_pos
    * 再根據此計算
        * n_tile_weights
        * n_w_slice

* tile_num = kernel_higth * kernel_width * tile_o_num * tile_i_num 
* tile_size 最大為32 * 16，由input_channel, output_channel控制
```
tile_size ==> 2048 = 2^11
Input size 5529 output size 4304 bpw 6.23
// 
START
checking1: n= 5529
n_tile_weights: 2048, n_w_slice: 1, total_w_bitcnt: 13307
n_tile_weights: 2048, n_w_slice: 2, total_w_bitcnt: 25858
n_tile_weights: 2048, n_w_slice: 3, total_w_bitcnt: 37361
Input size 5529 output size 4720 bpw 6.83

```
## Search for good "GRC parameters" for the "zrun" stream
* 有使用zero run的話:
    * 未啟用tile_mode: get n_z_slice
    * 啟用tile_mode: 
        * 先確認每個tile的z_start_pos
        * 再根據此計算
            * n_tile_zvalue
            * n_z_slice

## Encode bitstream slice
* 從所有tile中，依序對每个 Tile 的weight和zero run data 編碼(weight_values_tile_pos == tile所有的weight數量 == 該tile的結尾pos)
    * 用encode_slice對該tile的weight進行編碼
    * 更新weight和zero run的運行position

## 用到的function:
* search_grc_params(些微更動search_state_t *state[MAX_ZWCFG];)
* search_best_tile_grc_params
* encode_slice(些微更動for tile mode)

## mlw_encode的改變
* bitbuf_size多了3倍的inbuf_size
    * 不知道需不需要

* n_restarts直接設為1
    * 一個section only
* find_palette的改變: 可能是因為只要一個section，所以不用考慮分段的position
