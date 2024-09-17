# decode 256個weight
讓decode可以在得到固定數量(stop，目前預設256)的weight後，立即中斷並對其進行其他處理，完成後再回到原本的地方繼續decode，直到結束
## 構思
* 正常decode過程
    * state 0 時，會decode slice header的各參數
    * 完成後進入state 1，會掃過該slice內的每一個chunck，並計算出weight index
    * 直到該slice內所有elements decode完成
* 在過程中，處理到256個weight index時
    * 保存當時slice/chunck的參數，並將state改為2
    * 停止後直接進入到處理output的階段，並return
    * 再次呼叫時，會回到該chunck繼續decode下一個weight index
    * 該chunck結束後，state會跳回1，以便繼續下一個chunck


## state變化
* state 0: 讀取slice header & 初始化參數
* state 1: 掃過chunck，每次會讀取0~12個資料，直到將該slice轉換成weight index，直到處理累績256個或該slice處理完
* state 2: 當累績256個weight index，state將會從1變成2並輸出進行其他處理。待完成後，再次回到先前停止的地方，繼續將該chunck處理完，再次回到state 1
![image](https://hackmd.io/_uploads/HyW1ddNT0.png)

## 結果:
* decoder struct新增的變數
```
//for 256
	int w_carry;
	int state;
	int last;
	int w_enable_temp;
	int w_q_temp[12];
	int w_nsymbols_temp;
```
* decoder 初始化新增的變數
```
// for 256
    decoder->w_carry = 0;
    decoder->state = 0;
    decoder->last = 0;
    decoder->w_enable_temp = 0;
    memset(decoder->w_q_temp, 0, sizeof(decoder->w_q_temp));
	decoder->w_nsymbols_temp = 0;
```
* decode
    * stop: 要decode weight的數量
```
int mlw_decode(MLWDecoder *decoder, int stop)
```
* 結果:
![image](https://hackmd.io/_uploads/H1WUkIJ6A.png)



