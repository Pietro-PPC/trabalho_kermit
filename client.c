#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <string.h>
#include <errno.h>

#include <linux/if.h>
#include <sys/ioctl.h>
#include "ConexaoRawSocket.h"

#define DEVICE "lo"

void sendMessage(int sock, char *msg, struct sockaddr_ll *sockad){
    int len = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)sockad, sizeof (struct sockaddr_ll));
    if (len < 0){
        fprintf(stderr, "Deu ruim no sendto: %d\n", errno);
    }
}

int main(){
    struct sockaddr_ll sockad; 
    int sock = ConexaoRawSocket(DEVICE, &sockad);

    unsigned char *msg = "Hello, World!\n";
    sendMessage(sock, msg, &sockad);

    return 0;
}
