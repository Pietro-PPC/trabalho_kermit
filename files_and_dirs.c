#include <unistd.h>
#include <errno.h>

int executeCd(unsigned char *dir){
    if (!chdir(dir))
        return 0;
    return errno;
}
