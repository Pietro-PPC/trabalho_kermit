#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>

void sendMessage(int sock, char *msg, int msg_size, struct sockaddr_ll *sockad){
    int len = sendto(sock, msg, msg_size, 0, (struct sockaddr *)sockad, sizeof (struct sockaddr_ll));
    if (len < 0){
        fprintf(stderr, "Deu ruim no sendto: %d\n", errno);
    }
}

/*
  Retornos:
    0: Deu certo
    1: Deu problema
*/
int recieveMessage(int sock, unsigned char *buffer, int buf_size, struct sockaddr_ll *packet_info){
    int packet_info_size = sizeof(struct sockaddr_ll);
    int len = recvfrom(sock, buffer, buf_size, 0, (struct sockaddr *) packet_info, &packet_info_size);
    if (len < 0){
        fprintf(stderr, "Error recieving message!\n");
        return 1;
    }
    return 0;
}
