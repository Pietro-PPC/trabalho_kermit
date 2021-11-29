#include <stdio.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <unistd.h>

#include "ConexaoRawSocket.h"
#include "message.h"
#include "common.h"
#include "send_recieve.h"
#include "files_and_dirs.h"

#define DEVICE "lo"

int main(){

    struct sockaddr_ll sockad, packet_info;
    int sock = ConexaoRawSocket(DEVICE,&sockad);
    int len, ret;
    unsigned char buffer[BUF_SIZE], seq = 0;
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_parity, msg_data[MAX_MSG_SIZE];
    char response[MAX_MSG_SIZE];

    printf("Server is on...\n");
    for (;;){
        
        do { // Enquanto destino não é o server e a seq da mensagem não é a correta
            recieveMessage(sock, buffer, BUF_SIZE, &packet_info);
            ret = parseMsg(buffer, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
        } while (ret == 1 || msg_dst != SERVER_ADD || msg_sequence != seq); 
        
        
        buildAck(response, SERVER_ADD, CLIENT_ADD, seq);
        if (ret == 2){
            buildNack(response, SERVER_ADD, CLIENT_ADD, seq);
            seq--;
        }
        else if (msg_type == CD_TYPE){ // A partir daqui, temos o código das mensagens
            int command_ret = executeCd(msg_data);
            
            if (command_ret == 2){
                buildError(response, DIR_ER, seq);
            }
            else{
                char s[100];
                printf("%s\n", getcwd(s, 100));
            }
        }
        else if (msg_type == LS_TYPE){
            printf("ls\n");
        }
        else {
            fprintf(stderr, "Command unavailable!\n");
        }
        sendMessage(sock, response, MAX_MSG_SIZE, &sockad);
        seq = (seq+1) % MAX_SEQ;
    }

    printf("Server is off\n");
    return 0;
}
