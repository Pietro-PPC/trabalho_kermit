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
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_parity, msg_data[MAX_DATA_SIZE];
    int ret;

    while (strcmp(command, "exit")){
        getCommand(command);
        if (buildMsg(command, msg, seq))
            continue;
        do {
            sendMessage(sock, msg, MAX_MSG_SIZE, &sockad);
            // Pega mensagens até chegar uma endereçada ao client
            do {
                recieveMessage(sock, response, MAX_MSG_SIZE, &packet_info); // recebe resposta do server
                ret = parseMsg(response, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
            } while (ret == 1 || msg_dst != CLIENT_ADD || msg_sequence != seq);
            // Não sei se precisa mandar nack caso ret seja 2

            if (ret == 2) {
                fprintf(stderr, "Fatal error! Response message corrupted!\n");
                exit(1);
            }
        } while(msg_type == NACK_TYPE);

        if (msg_type == ACK_TYPE)
            printf("ACK!!!\n");
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
