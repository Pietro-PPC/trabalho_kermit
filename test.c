#include <stdio.h>
#include <string.h>

int main(){
    int i = 0;
    do{
        if (i == 3)
            continue;
        printf("%d\n", i);
    } while (++i < 5);
    return 0;
}
