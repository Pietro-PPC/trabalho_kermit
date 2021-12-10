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

void getChars(FILE *f, int charNum, unsigned char *buf){
    int i = 0;
    while( i < charNum && !feof(f)){
        buf[i++] = fgetc(f);
    }
    if (feof(f)) i--;
    buf[i] = 0;
}
/*
    Retornos: 
    . 0: Não acabou linha
    . 1: Acabou linha
    . 2: Acabou arquivo
*/
int getCharsLine(FILE *f, int charNum, unsigned char *buf){
    int i = 0, ret = 0;
    buf[i++] = fgetc(f);
    while (i < charNum && !feof(f) && buf[i-1] != '\n'){
        buf[i++] = fgetc(f);
    }
    if (feof(f) || buf[i-1] == '\n')
        ret = 1;
    if (feof(f)) 
        i--;
    buf[i] = 0;
    return ret;
}

/*
    Retornos:
        . 0: linha existe
        . 1: Linha não existe
*/
int iterateLineNum(FILE *f, int lineNum){
    int i = 1;
    char c;
    while (i < lineNum && !feof(f)){
        do{
            c = fgetc(f);
        } while (c != '\n' && !feof(f));
        ++i;
    }
    if (feof(f)) return 1;
    return 0;
}

/*
    Retornos: 
        0. Transmissão finalizou
        1. Transmissão não finalizou
*/
int buildFileMessages(msg_stream_t *msgStream, DIR *working_dir, int seq){
    struct dirent *cur_file;
    unsigned char msg[MAX_MSG_SIZE];

    resetMsgStream(msgStream);
    cur_file = readdir(working_dir);
    while(cur_file && msgStream->size < MAX_STREAM_LEN-1){
        if (strcmp(cur_file->d_name, ".") && strcmp(cur_file->d_name, "..")){
            seq = nextSeq(seq);
            buildLsFile(msg, cur_file->d_name, seq);
            pushMessage(msgStream, msg);
        }
        cur_file = readdir(working_dir);
    }
    seq = nextSeq(seq);
    if (cur_file){
        buildLsFile(msg, cur_file->d_name, seq);
        pushMessage(msgStream, msg);
        return 1;
    }
    buildEndTransmission(msg, SERVER_ADD, CLIENT_ADD, seq);
    pushMessage(msgStream, msg);
    return 0;
}

/*
    Retornos: 
        0. Transmissão finalizou
        1. Transmissão não finalizada
*/
int buildVerMessages(msg_stream_t *msgStream, FILE *f, int seq){
    unsigned char buf[MAX_DATA_SIZE+1], msg[MAX_MSG_SIZE], response[MAX_MSG_SIZE];
    resetMsgStream(msgStream);
    while (!feof(f) && msgStream->size < MAX_STREAM_LEN-1){
        getChars(f, MAX_DATA_SIZE, buf);
        seq = nextSeq(seq);
        buildFileContent(msg, buf, seq);
        pushMessage(msgStream, msg);
    }
    if (!feof(f)) return 1;

    seq = nextSeq(seq);
    buildEndTransmission(msg, SERVER_ADD, CLIENT_ADD, seq);
    pushMessage(msgStream, msg);
    return 0;
}

int buildLinhasMessages(msg_stream_t *msgStream, FILE *f,int *cur_lin, int last_lin, int seq){
    int lineEnds;
    unsigned char buf[MAX_DATA_SIZE], msg[MAX_MSG_SIZE];

    resetMsgStream(msgStream);
    while(*cur_lin <= last_lin && msgStream->size < MAX_STREAM_LEN-1){
        if (lineEnds = getCharsLine(f, MAX_DATA_SIZE, buf)){ // Chegou em final de linha
            if (*cur_lin == last_lin && !feof(f))
                buf[strlen(buf)-1] = 0;
            (*cur_lin)++;
        }
        seq = nextSeq(seq);
        buildFileContent(msg, buf, seq);
        pushMessage(msgStream, msg);
    }
    if (!lineEnds || *cur_lin <= last_lin)
        return 1;

    seq = nextSeq(seq);
    buildEndTransmission(msg, SERVER_ADD, CLIENT_ADD, seq);
    pushMessage(msgStream, msg);
    return 0;
}


int main(){

    struct sockaddr_ll sockad, packet_info;
    int sock = ConexaoRawSocket(DEVICE,&sockad);
    int len, ret;
    unsigned char buffer[BUF_SIZE], seq = 0;
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_parity, msg_data[MAX_DATA_SIZE+1];
    unsigned char response[MAX_MSG_SIZE];
    msg_stream_t msgStream;

    printf("Server is on...\n");
    getNextMessage(sock, buffer, SERVER_ADD, prevSeq(seq), 1);
    for (;;){
        if (LOG) notifyRecieve(buffer);
        ret = parseMsg(buffer, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);

        // Resposta padrão é um ACK para o comando recebido
        buildAck(response, SERVER_ADD, CLIENT_ADD, seq);
        while (ret == 2){
            buildNack(response, SERVER_ADD, CLIENT_ADD, seq);
            sendMessage(sock, response, MAX_MSG_SIZE, &sockad);
            getNextMessage(sock, buffer, SERVER_ADD, prevSeq(seq), 1);
            ret = parseMsg(buffer, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
        }
        if (msg_type == CD_TYPE){ // A partir daqui, temos o código das mensagens
            int command_ret = executeCd(msg_data);
            if (command_ret == 2 || command_ret == 20) // Diretório com esse nome não existe
                buildError(response, DIR_ER, seq); 
            else if (command_ret == 13){ // Sem permissão para acessar diretório.
                buildError(response, PERM_ER, seq);
            }
            sendMessage(sock, response, MAX_MSG_SIZE, &sockad);
        }
        else if (msg_type == LS_TYPE){
            // respondLs(sock, &sockad, &seq);
            DIR *working_dir = opendir("./");
            if (working_dir){
                do {
                    ret = buildFileMessages(&msgStream, working_dir, seq);
                    sendMultipleMsgs(sock, &msgStream, SERVER_ADD, &seq);
                } while (ret);
                closedir(working_dir);
            } else {
                buildError(response, DIR_ER, seq);
                sendMessage(sock, response, MAX_MSG_SIZE, NULL);
            }
        }
        else if (msg_type == VER_TYPE){
            FILE *f = fopen(msg_data, "r");
            if (f){
                do {
                    ret = buildVerMessages(&msgStream, f, seq);
                    sendMultipleMsgs(sock, &msgStream, SERVER_ADD, &seq);
                } while (ret);
                fclose(f);
            } else {
                buildError(response, FILE_ER, seq); // TODO: parsear o erro dentro da funcao
                sendMessage(sock, response, MAX_MSG_SIZE, NULL);
            }
        }
        else if (msg_type == LINHA_TYPE || msg_type == LINHAS_TYPE){
            FILE *f = fopen(msg_data, "r");
            int cur_lin, last_lin;
            if (f){
                // Manda ack para receber mensagem com linhas
                buildAck(response, SERVER_ADD, CLIENT_ADD, seq);
                sendMessage(sock, response, MAX_MSG_SIZE, NULL);

                // Recebe próxima mensagem e testa se linhas existem no arquivo
                getMessageInsist(sock, buffer, CLIENT_ADD, SERVER_ADD, seq);
                seq = nextSeq(seq);
                cur_lin = getFirstLineNum(buffer);
                last_lin = (msg_type == LINHA_TYPE) ? cur_lin : getLastLineNum(buffer);
                if (!iterateLineNum(f, last_lin)){
                    // Vai para linha inicial, constrói e manda mensagens
                    rewind(f);
                    iterateLineNum(f, cur_lin);
                    do {
                        ret = buildLinhasMessages(&msgStream, f, &cur_lin, last_lin, seq);
                        sendMultipleMsgs(sock, &msgStream, SERVER_ADD, &seq);
                    } while (ret);
                } else { // Linha inexistente
                    buildError(response, LINE_ER, seq);
                    sendMessage(sock, response, MAX_MSG_SIZE, NULL);
                }
                fclose(f);
            } else {
                // Arquivo inexistente
                buildError(response, FILE_ER, seq);
                sendMessage(sock, response, MAX_MSG_SIZE, NULL);
            } 
        }
        else {
            fprintf(stderr, "Command unavailable! %d\n", msg_type);
        }
        getNextMessage(sock, buffer, SERVER_ADD, seq, 1);
        seq = nextSeq(seq);
    }

    printf("Server is off\n");
    return 0;
}
