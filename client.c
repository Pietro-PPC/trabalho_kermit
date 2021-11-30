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
#include "files_and_dirs.h"
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
    struct sockaddr_ll sockad, packet_info; 
    int sock = ConexaoRawSocket(DEVICE, &sockad);

    char command[MAX_BUF] = "";
    unsigned char msg[MAX_MSG_SIZE], response[MAX_MSG_SIZE], seq;
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_parity, msg_data[MAX_DATA_SIZE+1];
    int ret;

    while (strcmp(command, "exit")){
        getCommand(command);
        if (buildMsgFromTxt(command, msg, seq))
            continue;

        sendMessageInsist(sock, msg, &sockad, response, CLIENT_ADD, seq);
        ret = parseMsg(response, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
        
        if (msg_type == ACK_TYPE)
            printf("ACK!!!\n");
        else if (msg_type == LS_CONT_TYPE){
            // Recebe todo conteúdo do ls e depois um fim_transmissão (Vamos assumir que sim)
            while(msg_type == LS_CONT_TYPE){ // se o ret for 2 pode ser que não seja realmente um ls
                if (ret == 2)
                    buildNack(msg, CLIENT_ADD, SERVER_ADD, seq);
                else{
                    printf("%s\n", msg_data);
                    buildAck(msg, CLIENT_ADD, SERVER_ADD, seq);
                    seq = (seq+1) % MAX_SEQ;
                }
                sendMessage(sock, msg, MAX_MSG_SIZE, &sockad);

                getNextMessage(sock, response, CLIENT_ADD, seq);
                ret = parseMsg(response, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
                if (ret == 2) msg_type = LS_CONT_TYPE; // Evitar que saia do loop com mensagem corrompida
            } 
            // Se não reconheceu ultima mensagem, 
            buildAck(msg, CLIENT_ADD, SERVER_ADD, seq);
            sendMessage(sock, msg, MAX_MSG_SIZE, &sockad);
        }
        else if (msg_type == ERROR_TYPE){
            switch (msg_data[0]){
                case 1:
                    fprintf(stderr, "Error: Permisison denied.\n");
                    break;
                case 2:
                    fprintf(stderr, "Error: No such directory.\n");
                    break;
                case 3:
                    fprintf(stderr, "Error: No such file\n");
                    break;
                case 4:
                    fprintf(stderr, "Error: No such line number\n");
                    break;
                default:
                    fprintf(stderr, "Got invalid error message\n");
            }
        }
        else
            fprintf(stderr, "Invalid message type\n");
        seq = (seq+1) % MAX_SEQ;
    }

    return 0;
}
