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
#include "common.h"

#define MAX_BUF 256
#define DEVICE "lo"

void sendMessage(int sock, char *msg, struct sockaddr_ll *sockad){
    int len = sendto(sock, msg, MAX_MSG_SIZE, 0, (struct sockaddr *)sockad, sizeof (struct sockaddr_ll));
    if (len < 0){
        fprintf(stderr, "Deu ruim no sendto: %d\n", errno);
    }
}

void get_command (char *command){
    printf(">>> ");
    fgets(command, MAX_BUF, stdin); 
    command[strcspn(command, "\n")] = '\0';
}

int main(){
    struct sockaddr_ll sockad; 
    int sock = ConexaoRawSocket(DEVICE, &sockad);

    char command[MAX_BUF];
    unsigned char msg[MAX_MSG_SIZE];

    get_command(command);
    while (strcmp(command, "exit")){
        msg[0] = 0x7E; // meio zoado estar aqui
        buildMsg(command, msg);
        printBitwise(msg, MAX_MSG_SIZE);
        sendMessage(sock, msg, &sockad);
        get_command(command);
    }

    return 0;
}
