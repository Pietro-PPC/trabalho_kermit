#include <stdio.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include "ConexaoRawSocket.h"

#define BUF_SIZE 2048

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

    int sock = ConexaoRawSocket("lo");
    int len;
    unsigned char buffer[BUF_SIZE];
    struct sockaddr_ll packet_info;
    int packet_info_size = sizeof(packet_info);

    if (len = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *) &packet_info, &packet_info_size) < 0){
        fprintf(stderr, "Deu ruim com o recvfrom\n");
        return 1;
    } else {
        printPacket(buffer, len);
    }

    // for (;;){
    //     printf("Waiting for connection...\n");
    //     fflush(stdout);
    //     int new_sock = accept(sock, (struct sockaddr *)NULL, NULL);
    // }

    return 0;
}
