#include <stdio.h>
#include <string.h>

int main(){
    char *msg = "A string";
    printf("res = %ld\n", strcspn(msg, "a"));
    return 0;
}
