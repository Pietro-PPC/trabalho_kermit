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
#include <dirent.h>
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

void printFileContent(unsigned char *msg_data, int *line, int withLineNum){
    char *c = msg_data;
    while (*c){
        putchar(*c);
        if (*c == '\n' && withLineNum)
            printf("%3d ", ++(*line));
        c++;
    }
}

void printLines(msg_stream_t *msgStream, int* cur_lin, int withLineNum){
    unsigned char data[MAX_MSG_SIZE+1];
    for (int i = 0; i < msgStream->size; ++i){
        getMsgData(msgStream->stream[i], data);
        printFileContent(data, cur_lin, withLineNum);
    }
}

int printFiles(msg_stream_t *msgStream){
    unsigned char data[MAX_MSG_SIZE+1];
    for (int i = 0; i < msgStream->size; ++i){
        getMsgData(msgStream->stream[i], data);
        printf("%s\n", data);
    }
}

/*
    Retornos:
        0. Tudo certo
        1. Erro ao abrir diretório
*/
int executeLls(){
    DIR *working_dir = opendir("./");
    struct dirent *cur_file;
    if (working_dir){
        while(cur_file = readdir(working_dir))
            if (strcmp(cur_file->d_name, ".") && strcmp(cur_file->d_name, ".."))
                printf("%s\n", cur_file->d_name);
        return 0;
    }
    return 1;
}

int main(){
    struct sockaddr_ll sockad;
    int sock = ConexaoRawSocket(DEVICE, &sockad);
    struct timespec max_wait;
    char promptLine[MAX_BUF] = "", command[MAX_BUF], buf[MAX_BUF];
    unsigned char msg[MAX_MSG_SIZE], response[MAX_MSG_SIZE], seq;
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_parity, msg_data[MAX_DATA_SIZE+1];
    int ret, reps, err, lin_ini, timeout = 0;
    msg_stream_t msgStream;

    max_wait.tv_sec = 2;
    max_wait.tv_nsec = 0;
    resetMsgStream(&msgStream);

    // Parâmetros fixos de cada função de timeout
    struct sendInsistParams siParams;
    siParams.sock = sock;
    siParams.sockad = NULL;
    siParams.response = response;
    siParams.addr = CLIENT_ADD;

    struct getInsistParams giParams;
    giParams.sock = sock;
    giParams.src = SERVER_ADD;
    giParams.dest = CLIENT_ADD;
    giParams.msg = response;

    struct getMultipleParams gmParams;
    gmParams.sock = sock;
    gmParams.src = SERVER_ADD;
    gmParams.dest = CLIENT_ADD;
    gmParams.seq = &seq;
    gmParams.ret = &ret;
    gmParams.s = &msgStream;

    for (;;){
        getPromptLine(promptLine);
        if (!strcmp(promptLine, "exit"))
            break;

        sscanf(promptLine, "%s", command);
        if (!strcmp(command, LLS_STR)){
            ret = executeLls();
            if (ret) printError(DIR_ER);
            continue;
        }
        if (!strcmp(command, LCD_STR)){
            sscanf(promptLine+strlen(LCD_STR), "%s", buf);
            ret = executeCd(buf);
            if (ret == 2 || ret == 20)
                printError(DIR_ER); 
            else if (ret == 13) // Sem permissão para acessar diretório.
                printError(PERM_ER);
            continue;
        }
        if (err = buildMsgsFromTxt(promptLine, &msgStream, seq)){
            fprintf(stderr, "Error with command!\n");
            continue;
        }
        ret = 0;
        msg_type = NEUTRAL_TYPE;
        for (int i = 0; i < msgStream.size && !ret && msg_type != ERROR_TYPE; ++i){
            siParams.msg = msgStream.stream[i];
            siParams.seq = seq;
            if (executeOrTimeout(&sendMessageInsistTimeout, &siParams, &max_wait) == -1){
                fprintf(stderr, "Timeout\n"); return 1;
            }
            ret = parseMsg(siParams.response, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
            seq = nextSeq(seq); 
        }
        seq = prevSeq(seq);

        if (!strcmp(command, COMPILAR_STR)){ // Pega retorno do gcc
            giParams.seq = seq;
            if (executeOrTimeout(&getMessageInsistTimeout, &giParams, &max_wait) == -1){
                fprintf(stderr, "Timeout\n"); return 1;
            }
            ret = parseMsg(giParams.msg, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
        }

        if (msg_type == ACK_TYPE){
            printf("Done\n");
        }
        else if (msg_type == LS_CONT_TYPE){
            seq = nextSeq(seq);
            resetMsgStream(&msgStream);
            pushMessage(&msgStream, response);
            buildAck(msg, CLIENT_ADD, SERVER_ADD, seq);
            sendMessage(sock, msg, MAX_MSG_SIZE, NULL);
            do{
                if (executeOrTimeout(&getMultipleMsgssTimeout, &gmParams, &max_wait)){
                    fprintf(stderr, "Timeout"); return 1;
                }
                printFiles(&msgStream);
                resetMsgStream(&msgStream);
            } while (ret);
        }
        else if (msg_type == FILE_CONT_TYPE ){
            int withLineNum = strcmp(command, COMPILAR_STR) ? 1 : 0;
            lin_ini = 1;
            if (!strcmp(command, LINHA_STR) || !strcmp(command, LINHAS_STR))
                lin_ini = getFirstLineNum(msgStream.stream[1]); // Última mensagem mandada tem numero da linha
            
            seq = nextSeq(seq);
            resetMsgStream(&msgStream); 
            pushMessage(&msgStream, response);
            buildAck(msg, CLIENT_ADD, SERVER_ADD, seq);
            sendMessage(sock, msg, MAX_MSG_SIZE, NULL);

            if (withLineNum) printf("%3d ", lin_ini);
            do {
                if (executeOrTimeout(&getMultipleMsgssTimeout, &gmParams, &max_wait)){
                    fprintf(stderr, "Timeout"); return 1;
                }
                printLines(&msgStream, &lin_ini, withLineNum);
                resetMsgStream(&msgStream);
            } while(ret);
            printf("\n");
        }
        else if (msg_type == ERROR_TYPE){
            printError(msg_data[0]);
        }
        else if (msg_type == END_TRANSM_TYPE){
            seq = nextSeq(seq);
            buildAck(msg, CLIENT_ADD, SERVER_ADD, seq);
            sendMessage(sock, msg, MAX_MSG_SIZE, NULL);
        }
        else{
            fprintf(stderr, "Invalid response from server\n");
        }
        seq = nextSeq(seq);
    }

    return 0;
}
