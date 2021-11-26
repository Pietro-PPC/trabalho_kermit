#include <stdio.h>
#include <string.h>

int main(){
    unsigned char parsed_msg[19];
    parsed_msg[0] = 126;
    parsed_msg[0] <<= 1;
    memset(parsed_msg+1, 0, sizeof(parsed_msg)-1);
    printf("parsed_msg: %d\n", *parsed_msg);
    return 0;
}
