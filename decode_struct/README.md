# struct decode
## 將原本的mlw_decode.c的函數轉換成struct
## 將該儲存的變數放到struct中
# 使用
* 先編譯
```
gcc -o mlw_codec mlw_main.c mlw_encode.c mlw_decode.c -lm
```

* decode
```
./mlw_codec -d result.txt -o decode.txt
```
