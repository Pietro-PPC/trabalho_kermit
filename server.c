#include <stdio.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include "ConexaoRawSocket.h"
#include "message.h"

#define BUF_SIZE 2048
#define DEVICE "lo"

int main(){

    struct sockaddr_ll sockad; 
    int sock = ConexaoRawSocket(DEVICE,&sockad);
    int len;
    unsigned char buffer[BUF_SIZE];
    struct sockaddr_ll packet_info;
    int packet_info_size = sizeof(packet_info);

    for (int i = 0; i < 2; ++i){
        if ( (len = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *) &packet_info, &packet_info_size)) < 0){
            fprintf(stderr, "No message!\n");
            return 1;
        } else 
            parseMsg(buffer);
    }

    return 0;
}
