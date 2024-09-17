# 使用
* 先編譯
```
gcc -o mlw_codec mlw_main.c mlw_encode.c mlw_decode.c -lm
```
* encode
```
./mlw_codec test.txt -o result.txt
```
* decode
```
./mlw_codec -d result.txt -o decode.txt
```
