#include <stdio.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include "ConexaoRawSocket.h"

#define BUF_SIZE 2048
#define DEVICE "lo"

void printPacket(unsigned char *buf, int len){
    unsigned char *p = buf;

    printf("\nPackage start\n");

    while (len--){
        printf("%c\n", *p);
        p++;
    }

    printf("\nPackage end\n");
}

int main(){

    struct sockaddr_ll sockad; 
    int sock = ConexaoRawSocket(DEVICE,&sockad);
    int len;
    unsigned char buffer[BUF_SIZE];
    struct sockaddr_ll packet_info;
    int packet_info_size = sizeof(packet_info);


    for (;;){
        printf("Waiting for connection...\n");
        fflush(stdout);

        if ( (len = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *) &packet_info, &packet_info_size)) < 0){
            fprintf(stderr, "Deu ruim com o recvfrom\n");
            return 1;
        } else if (len){
            printPacket(buffer, len);
        }
    }

    return 0;
}
