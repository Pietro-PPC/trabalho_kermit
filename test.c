#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main(){
    char dir[100];
    scanf("%s", dir);
    int ret = 0;
    if (chdir(dir))
        ret = errno;
    printf("ret: %d\n", ret);
}
