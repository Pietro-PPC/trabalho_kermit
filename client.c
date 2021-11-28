#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/if.h>

#include "ConexaoRawSocket.h"
#include "message.h"
#include "common.h"
#include "send_recieve.h"

#define MAX_BUF 256
#define DEVICE "lo"


void getCommand (char *command){
    do {
        printf(">>> ");
        fgets(command, MAX_BUF, stdin); 
        command[strcspn(command, "\n")] = '\0';
    } while(!strlen(command));
}

int main(){
    struct sockaddr_ll sockad; 
    int sock = ConexaoRawSocket(DEVICE, &sockad);

    char command[MAX_BUF];
    unsigned char msg[MAX_MSG_SIZE];

    getCommand(command);
    while (strcmp(command, "exit")){
        buildMsg(command, msg);
        printBitwise(msg, MAX_MSG_SIZE);
        sendMessage(sock, msg, MAX_MSG_SIZE, &sockad);
        getCommand(command);
    }

    return 0;
}
