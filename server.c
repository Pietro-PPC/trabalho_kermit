#include <stdio.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#include "ConexaoRawSocket.h"
#include "message.h"
#include "common.h"
#include "send_recieve.h"
#include "files_and_dirs.h"
#include "send_recieve.h"

#define DEVICE "lo"

void respondLs(int sock, struct sockaddr_ll *sockad, unsigned char *seq){
    DIR *working_dir;
    struct dirent *cur_file;
    struct sockaddr_ll packet_info;
    unsigned char msg[MAX_MSG_SIZE], response[MAX_MSG_SIZE], buf[MAX_MSG_SIZE];

    working_dir = opendir("./");
    if (working_dir){
        while(cur_file = readdir(working_dir)){
            if (!strcmp(cur_file->d_name, ".") || !strcmp(cur_file->d_name, ".."))
                continue;
            buildLsFile(msg, cur_file->d_name, *seq);
            sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);
            *seq = (*seq+1) % MAX_SEQ;
        }
        // Ordenar se der tempo
        closedir(working_dir);
    }// TODO: Criar else
    buildEndTransmission(msg, SERVER_ADD, CLIENT_ADD, *seq);
    sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);
}

/*
    Retorna quantos caracteres foram passados para buf
*/
void getChars(FILE *f, int charNum, unsigned char *buf){
    int i = 0;
    while( i < charNum && !feof(f)){
        buf[i++] = fgetc(f);
    }
    if (feof(f)) i--;
    buf[i] = 0;
}

void respondVer(int sock, struct sockaddr_ll *sockad, unsigned char *seq, unsigned char *filename){
    // ler 15 por 15 caracteres da mensagem até encontrar EOF
        // Mandar 15 caracteres
    unsigned char buf[MAX_DATA_SIZE+1], msg[MAX_MSG_SIZE], response[MAX_MSG_SIZE];
    
    FILE *f = fopen(filename, "r");
    if (!f){
        buildError(msg, 1, *seq); // TODO: parsear o erro dentro da funcao
        sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);
        return;
    }

    while (!feof(f)){
        getChars(f, MAX_DATA_SIZE, buf);
        buildFileContent(msg, buf, *seq);
        sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);
        *seq = (*seq+1) % MAX_SEQ;
    }
    buildEndTransmission(msg, SERVER_ADD, CLIENT_ADD, *seq);
    sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);

    fclose(f);
}

int main(){

    struct sockaddr_ll sockad, packet_info;
    int sock = ConexaoRawSocket(DEVICE,&sockad);
    int len, ret;
    unsigned char buffer[BUF_SIZE], seq = 0;
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_parity, msg_data[MAX_MSG_SIZE];
    char response[MAX_MSG_SIZE];

    printf("Server is on...\n");
    for (;;){
        getNextMessage(sock, buffer, SERVER_ADD, seq);
        ret = parseMsg(buffer, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);

        // Resposta padrão é um ACK
        buildAck(response, SERVER_ADD, CLIENT_ADD, seq);
        if (ret == 2){
            buildNack(response, SERVER_ADD, CLIENT_ADD, seq);
        }
        else if (msg_type == CD_TYPE){ // A partir daqui, temos o código das mensagens
            int command_ret = executeCd(msg_data);
            
            if (command_ret == 2 || command_ret == 20) // Diretório com esse nome não existe
                buildError(response, DIR_ER, seq); 
            else if (command_ret == 13){ // Sem permissão para acessar diretório.
                buildError(response, PERM_ER, seq);
            }
            seq = (seq+1) % MAX_SEQ;
        }
        else if (msg_type == LS_TYPE){
            respondLs(sock, &sockad, &seq);
            seq = (seq+1) % MAX_SEQ;
        }
        else if (msg_type == VER_TYPE){
            respondVer(sock, &sockad, &seq, msg_data);
            seq = (seq+1) % MAX_SEQ;
        }
        else {
            fprintf(stderr, "Command unavailable! %d\n", msg_type);
        }
        sendMessage(sock, response, MAX_MSG_SIZE, &sockad);
        
    }

    printf("Server is off\n");
    return 0;
}
