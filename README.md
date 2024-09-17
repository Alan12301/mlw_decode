# MLW Decoder
## Decode step
### **Reduce to Weight Index**
* compressed weight stream輸入進來之後，由stream的開頭照順序deocde到結尾(第一個section的第一個slice到最後section的最後slice)
* 讀進palette以及slice的header之後，接下來會**decode slice內部的chuncks**(詳見如下)

### **Golomb-Rice Decode**
* 同上面encode的例子
* 資料送進decoder時，對deocder來說，只有**unary0的長度是已知的(12bits)**，所以**第一步必須先確定unary1的長度**
* 由編碼的規則已知只要**unary0編碼出1(代表還沒到商數編碼結尾)，unary1就會編碼一個數**(可能是0也可能是1，0就代表該商數的編碼結尾)
    * 因此先掃過unary0，得出共有幾個1，進而確認unary1的長度
* 確認unary0以及unary1長度後，就可以從unary0以及unary1**逆推回去確認這chunk當中的壓縮整數的商數**
* 得到這個chunk的所有商數，就可以**跟後面對應的reaminder相加，還原出weight index**
* 此外這個例子有跨兩個chunk的case(前面編碼時的商數10)，在decode時會把**跨越的部分用register保留**起來，在下個chunk時才進行decode


![](https://casmd.ee.ncku.edu.tw/uploads/e634361496cf87911fcac9822.png)

![](https://casmd.ee.ncku.edu.tw/uploads/e634361496cf87911fcac9825.png)

---

### **Reduce to u_weight**
* 得到weight index之後，**根據slice的header**，可以得到這個slice有無採用zero runlength，並採取相對應的做法，還原出u_weight
    * w_div 
    * z_div

### **Palette/Direct Decode**
* 若本段slice採用palette mode, 則會進行palette decode，
    * 對於weight index <= palette size(mlw codec 是32) (存在palette中)
        * u_weight = inverse_palette[weight index]
    * 否則 direct decode
        * u_weight = weight index - palette size + dirofs
* 若本段slice沒有palette，直接進行direct decode，規則同上(palette size = 0)

**Zero Runlength Decode**
(待補)

---
### **Reduce to weight**
* 得到u_weight之後，只需要按照之前的轉換規則**反向回推到int9**的值域即可得到最初的weight值
* 最後將**int9的值用int16表達**，並組合在一起就可以得到原始的int16 weight stream
