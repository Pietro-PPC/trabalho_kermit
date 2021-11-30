#include <stdio.h>

// void getCurDirectory(char *dir_name){
//     char delim = "/";
//     char *cur_ptr = strtok(dir_name, delim);
//     char *last_ptr = cur_ptr;
//     while (cur_ptr){
//         last_ptr = cur_ptr;
//         cur_ptr = strtok(NULL, delim);
//     }
//     strcpy(dir_name, last_ptr);
// }

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

void printPacket(unsigned char *buf, int len){
    unsigned char *p = buf;

    printf("\nPackage start\n");

    while (len--){
        printf("%c", *p);
        p++;
    }

    printf("\nPackage end\n");
}

int min(int a, int b){
    if (a < b)
        return a;
    return b;
}
