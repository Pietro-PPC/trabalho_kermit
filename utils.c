#include <stdio.h>

void printBitwise(unsigned char *txt, int len){
    unsigned char *a = txt, mask;
    for (int i = 0; i < len; ++i, ++a){
        for (mask = 128; mask; mask >>= 1){
            printf("%d", *a & mask ? 1 : 0);
        }
        printf(" ");
    }
    printf("\n");
}