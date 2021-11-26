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
#include "message.h"
#include "utils.h"

#define DEVICE "lo"

void sendMessage(int sock, char *msg, struct sockaddr_ll *sockad){
    int len = sendto(sock, msg, MAX_MSG_SIZE, 0, (struct sockaddr *)sockad, sizeof (struct sockaddr_ll));
    if (len < 0){
        fprintf(stderr, "Deu ruim no sendto: %d\n", errno);
    }
}

int main(){
    struct sockaddr_ll sockad; 
    int sock = ConexaoRawSocket(DEVICE, &sockad);

    unsigned char *msg = "cd hotmaper";
    unsigned char parsed[MAX_MSG_SIZE];
    parsed[0] = 126;
    parseMsg(msg, parsed);
    printBitwise(parsed, MAX_MSG_SIZE);
    sendMessage(sock, msg, &sockad);

    return 0;
}
