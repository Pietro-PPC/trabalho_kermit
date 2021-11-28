#include <stdio.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>

#include "ConexaoRawSocket.h"
#include "message.h"
#include "common.h"
#include "send_recieve.h"

#define DEVICE "lo"

int main(){

    struct sockaddr_ll sockad; 
    int sock = ConexaoRawSocket(DEVICE,&sockad);
    int len, ret;
    unsigned char buffer[BUF_SIZE];
    struct sockaddr_ll packet_info;
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_parity;
    char response[MAX_MSG_SIZE];

    printf("Server is on...\n");
    for (;;){
        recieveMessage(sock, buffer, BUF_SIZE, &packet_info);
        
        ret = parseMsg(buffer, &msg_dst, &msg_size, &msg_sequence, &msg_type, &msg_parity);
        if (ret == 1 || msg_dst != SERVER_ADD)
            continue;
        
        // printBitwise(buffer, MAX_MSG_SIZE);
        buildAck(response, SERVER_ADD, CLIENT_ADD);
        if (ret == 2){
            buildNack(response, SERVER_ADD, CLIENT_ADD);
            sendMessage(sock, response, MAX_MSG_SIZE, &sockad);
        }
        else if (msg_type == CD_TYPE){ // A partir daqui, temos o código das mensagens
            printf("cd\n");
            sendMessage(sock, response, MAX_MSG_SIZE, &sockad);
        }
        else if (msg_type == LS_TYPE){
            printf("ls\n");
            sendMessage(sock, response, MAX_MSG_SIZE, &sockad);
        }
        else {
            fprintf(stderr, "Command unavailable!\n");
        }
    }

    printf("Server is off\n");
    return 0;
}
