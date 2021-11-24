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


#define DEST "127.0.0.1"
#define DEVICE "lo"

#define ADDR_0 0x0F
#define ADDR_1 0x00
#define ADDR_2 0x00
#define ADDR_3 0x01

int main(){
    unsigned char *hello = "Hello, World!Hello, World!Hello, World!Hello, World!Hello, World!Hello, World!\n";
    int sock = ConexaoRawSocket(DEVICE);
    struct sockaddr_ll saddr;

    memset(&saddr, 0, sizeof(saddr));

    saddr.sll_family = AF_PACKET;
    saddr.sll_protocol = htons(ETH_P_ALL);
    saddr.sll_addr[0] = ADDR_0;
    saddr.sll_addr[1] = ADDR_1;
    saddr.sll_addr[2] = ADDR_2;
    saddr.sll_addr[3] = ADDR_3;
    saddr.sll_halen = 4;

    struct ifreq ir;
    memset(&ir, 0, sizeof(struct ifreq));  	
    memcpy(ir.ifr_name, DEVICE, sizeof(DEVICE));
    if (ioctl(sock, SIOCGIFINDEX, &ir) == -1) {
        printf("Erro no ioctl\n");
        exit(-1);
    }
    saddr.sll_ifindex = ir.ifr_ifindex;

    int len = sendto(sock, hello, strlen(hello), 0, (struct sockaddr *)&saddr, sizeof(struct sockaddr_ll));
    if (len < 0){
    memset(&saddr, 0, sizeof(saddr));
        fprintf(stderr, "Deu ruim no sendto: %d\n", errno);
    }
    return 0;
}
