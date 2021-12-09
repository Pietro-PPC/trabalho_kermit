/*
TODO: 
    . Retorno de erro quanto arquivo não existe
    . Alguns outros retornos de erro, meio q tem q ver tudo
    . Timeout
    . Linhas negativas permitidas
    . Testar no ver se arquivo é diretório
    . Testar se segunda mensagem do linha/linhas é mesmo numeros de linha
    . Conferir mensagens nas rotinas do server
*/

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


void getPromptLine (char *promptLine){
    do {
        printf(">>> ");
        fgets(promptLine, MAX_BUF, stdin); 
        promptLine[strcspn(promptLine, "\n")] = '\0';
    } while(!strlen(promptLine));
}

void printDirectory(unsigned char *msg_data){
    printf("%s\n", msg_data);
}

void printFileContent(unsigned char *msg_data, int *line){
    char *c = msg_data;
    while (*c){
        putchar(*c);
        if (*c == '\n')
            printf("%3d ", ++(*line));
        c++;
    }
}

// falta especificar a linha que vai começar
void getMultipleMsgs(int sock, unsigned char *response, unsigned char *seq, struct sockaddr_ll *sockad, int type, int line){
    // Recebe todo conteúdo do ls e depois um fim_transmissão (Vamos assumir que sim)
    int ret;
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_parity, msg_data[MAX_DATA_SIZE+2];
    unsigned char msg[MAX_MSG_SIZE];

    if (type == FILE_CONT_TYPE)
        printf("%3d ", line);

    ret = parseMsg(response, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
    while(msg_type == type){
        if (ret == 2)
            buildNack(msg, CLIENT_ADD, SERVER_ADD, *seq);
        else{
            if (type == LS_CONT_TYPE)
                printDirectory(msg_data);
            else if (type == FILE_CONT_TYPE)
                printFileContent(msg_data, &line);
            
            buildAck(msg, CLIENT_ADD, SERVER_ADD, *seq);
            *seq = (*seq+1) % MAX_SEQ;
        }
        sendMessage(sock, msg, MAX_MSG_SIZE, sockad);

        getNextMessage(sock, response, CLIENT_ADD, *seq, 0);
        ret = parseMsg(response, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
        if (ret == 2) msg_type = type; // Evitar que saia do loop com mensagem corrompida
    } 
    buildAck(msg, CLIENT_ADD, SERVER_ADD, *seq);
    sendMessage(sock, msg, MAX_MSG_SIZE, sockad);
}

void printLines(msg_stream_t *msgStream, int* cur_lin){
    unsigned char data[MAX_MSG_SIZE+1];
    for (int i = 0; i < msgStream->size; ++i){
        getMsgData(msgStream->stream[i], data);
        printFileContent(data, cur_lin);
    }
}

int printFiles(msg_stream_t *msgStream){
    unsigned char data[MAX_MSG_SIZE+1];
    for (int i = 0; i < msgStream->size; ++i){
        getMsgData(msgStream->stream[i], data);
        printf("%s\n", data);
    }
}


int main(){
    struct sockaddr_ll sockad, packet_info; 
    int sock = ConexaoRawSocket(DEVICE, &sockad);

    char promptLine[MAX_BUF] = "", command[MAX_BUF];
    unsigned char msg[MAX_MSG_SIZE], response[MAX_MSG_SIZE], seq;
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_parity, msg_data[MAX_DATA_SIZE+1];
    int ret, reps, err, lin_ini;
    msg_stream_t msgStream;
    resetMsgStream(&msgStream);

    for (;;){
        getPromptLine(promptLine);
        if (!strcmp(promptLine, "exit"))
            break;

        sscanf(promptLine, "%s", command);
        reps = 1;
        if (!strcmp(command, LINHA_STR) || !strcmp(command, LINHAS_STR))
            reps = 2;

        err = 0;
        msg_type = NEUTRAL_TYPE;

        for (int i = 1; i <= reps && !err && msg_type != ERROR_TYPE; ++i){
            if (err = buildMsgFromTxt(promptLine, msg, seq, i)){
                fprintf(stderr, "Command not found!\n");
            }
            else {
                sendMessageInsist(sock, msg, &sockad, response, CLIENT_ADD, seq);
                if (i < reps) seq = (seq+1) % MAX_SEQ; 
                ret = parseMsg(response, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
            }
        }
        if (err) continue;

        if (msg_type == ACK_TYPE)
            printf("ACK!!!\n");
        else if (msg_type == LS_CONT_TYPE){
            // getMultipleMsgs(sock, response, &seq, &sockad, LS_CONT_TYPE, 0);
            seq = nextSeq(seq);
            resetMsgStream(&msgStream);
            pushMessage(&msgStream, response);
            buildAck(msg, CLIENT_ADD, SERVER_ADD, seq);
            sendMessage(sock, msg, MAX_MSG_SIZE, NULL);
            do{
                ret = getMultipleMsgss(sock, &msgStream, SERVER_ADD, CLIENT_ADD, &seq);
                printFiles(&msgStream);
                resetMsgStream(&msgStream);
            } while (ret);
        }
        else if (msg_type == FILE_CONT_TYPE){
            lin_ini = 1;
            if (!strcmp(command, LINHA_STR) || !strcmp(command, LINHAS_STR))
                lin_ini = getFirstLineNum(msg); // Última mensagem mandada tem numero da linha
            
            seq = nextSeq(seq);
            resetMsgStream(&msgStream);
            pushMessage(&msgStream, response);
            buildAck(msg, CLIENT_ADD, SERVER_ADD, seq);
            sendMessage(sock, msg, MAX_MSG_SIZE, NULL);

            printf("%3d ", lin_ini);
            do {
                ret = getMultipleMsgss(sock, &msgStream, SERVER_ADD, CLIENT_ADD, &seq);
                printLines(&msgStream, &lin_ini);
                resetMsgStream(&msgStream);
            } while(ret);
            printf("\n");
        }
        else if (msg_type == ERROR_TYPE){
            switch (msg_data[0]){
                case PERM_ER:
                    fprintf(stderr, "Error: Permisison denied.\n");
                    break;
                case DIR_ER:
                    fprintf(stderr, "Error: No such directory.\n");
                    break;
                case FILE_ER:
                    fprintf(stderr, "Error: No such file\n");
                    break;
                case LINE_ER:
                    fprintf(stderr, "Error: No such line number\n");
                    break;
                default:
                    fprintf(stderr, "Got invalid error message\n");
            }
        }
        else if (msg_type == END_TRANSM_TYPE){
            seq = nextSeq(seq);
            buildAck(msg, CLIENT_ADD, SERVER_ADD, seq);
            sendMessage(sock, msg, MAX_MSG_SIZE, NULL);
        }
        else
            fprintf(stderr, "Invalid message type\n");
        seq = (seq+1) % MAX_SEQ;
    }

    return 0;
}
