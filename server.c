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
            *seq = nextSeq(*seq);
            buildLsFile(msg, cur_file->d_name, *seq);
            sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);
        }
        // Ordenar se der tempo
        closedir(working_dir);
    }// TODO: Criar else
    *seq = nextSeq(*seq);
    buildEndTransmission(msg, SERVER_ADD, CLIENT_ADD, *seq);
    sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);
}

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

void respondVer(int sock, struct sockaddr_ll *sockad, unsigned char *seq, unsigned char *filename){
    unsigned char buf[MAX_DATA_SIZE+1], msg[MAX_MSG_SIZE], response[MAX_MSG_SIZE];

    FILE *f = fopen(filename, "r");
    if (!f){
        buildError(msg, 1, *seq); // TODO: parsear o erro dentro da funcao
        sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);
        return;
    }

    while (!feof(f)){
        getChars(f, MAX_DATA_SIZE, buf);
        *seq = nextSeq(*seq);
        buildFileContent(msg, buf, *seq);
        sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);
    }
    *seq = nextSeq(*seq);
    buildEndTransmission(msg, SERVER_ADD, CLIENT_ADD, *seq);
    sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);

    fclose(f);
}

void respondLinha(int sock, struct sockaddr_ll *sockad, unsigned char *seq, unsigned char *filename){
    unsigned char buf[MAX_DATA_SIZE+1], msg[MAX_MSG_SIZE], response[MAX_MSG_SIZE];
    int fimMensagem, lineNum;
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_parity, msg_data[MAX_DATA_SIZE+1];
    
    FILE *f = fopen(filename, "r");
    if (!f){
        buildError(msg, FILE_ER, *seq); // TODO: parsear o erro dentro da funcao
        sendMessage(sock, msg, MAX_MSG_SIZE, sockad);
        *seq = (*seq+1) % MAX_SEQ; // Isso deveria ser eliminado da face da terra mas deixa assim por enquanto
        return;
    }
    buildAck(msg, SERVER_ADD, CLIENT_ADD, *seq);
    sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);
    *seq = nextSeq(*seq); // acho q era pra ser sendmessage so

    getNextMessage(sock, msg, SERVER_ADD, *seq, 0);
    if (iterateLineNum(f, getFirstLineNum(msg))){
        buildError(msg, LINE_ER, *seq);
        sendMessage(sock, msg, MAX_MSG_SIZE, sockad);
        return;
    }

    fimMensagem = 0;
    while (!fimMensagem){
        fimMensagem = getCharsLine(f, MAX_DATA_SIZE, buf);
        if (fimMensagem && !feof(f)) buf[strlen(buf)-1] = 0;
        buildFileContent(msg, buf, *seq);
        sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);
        *seq = (*seq+1) % MAX_SEQ;
    }
    buildEndTransmission(msg, SERVER_ADD, CLIENT_ADD, *seq);
    sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);

    fclose(f);
}

void respondLinhas(int sock, struct sockaddr_ll *sockad, unsigned char *seq, unsigned char *filename){
    unsigned char buf[MAX_DATA_SIZE+1], msg[MAX_MSG_SIZE], response[MAX_MSG_SIZE];
    int fimMensagem, cur_line, first_line, last_line;
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_parity, msg_data[MAX_DATA_SIZE+1];
    
    // Testa se arquivo existe
    FILE *f = fopen(filename, "r");
    if (!f){
        buildError(msg, FILE_ER, *seq); // TODO: parsear o erro dentro da funcao
        sendMessage(sock, msg, MAX_MSG_SIZE, sockad);
        *seq = nextSeq(*seq); // Isso deveria ser eliminado da face da terra mas deixa assim por enquanto
        return;
    }
    // Confirma que arquivo existe
    buildAck(msg, SERVER_ADD, CLIENT_ADD, *seq);
    sendMessageInsist(sock, msg, sockad, response, SERVER_ADD, *seq);
    *seq = nextSeq(*seq);

    // Recebe linhas e testa se existem
    getNextMessage(sock, msg, SERVER_ADD, *seq, 1);
    first_line = getFirstLineNum(msg);
    last_line = getLastLineNum(msg);
    if (iterateLineNum(f, last_line)){
        buildError(msg, LINE_ER, *seq);
        sendMessage(sock, msg, MAX_MSG_SIZE, sockad);
        return;
    }
    rewind(f);
    iterateLineNum(f,first_line);

    // Devolve linhas
    cur_line = first_line;
    while(cur_line <= last_line){
        if (getCharsLine(f, MAX_DATA_SIZE, buf)){ // Chegou em final de linha
            if (cur_line == last_line && !feof(f))
                buf[strlen(buf)-1] = 0;
            cur_line++;
        }
        
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
    getNextMessage(sock, buffer, SERVER_ADD, prevSeq(seq), 1);
    for (;;){
        notifyRecieve(buffer);
        ret = parseMsg(buffer, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);

        // Resposta padrão é um ACK para o comando recebido
        buildAck(response, SERVER_ADD, CLIENT_ADD, seq);
        if (ret == 2){
            buildNack(response, SERVER_ADD, CLIENT_ADD, seq);
            sendMessage(sock, response, MAX_MSG_SIZE, &sockad);
        }
        else if (msg_type == CD_TYPE){ // A partir daqui, temos o código das mensagens
            int command_ret = executeCd(msg_data);
            
            if (command_ret == 2 || command_ret == 20) // Diretório com esse nome não existe
                buildError(response, DIR_ER, seq); 
            else if (command_ret == 13){ // Sem permissão para acessar diretório.
                buildError(response, PERM_ER, seq);
            }
            sendMessage(sock, response, MAX_MSG_SIZE, &sockad);
        }
        else if (msg_type == LS_TYPE){
            respondLs(sock, &sockad, &seq);
        }
        else if (msg_type == VER_TYPE){
            respondVer(sock, &sockad, &seq, msg_data);
        }
        else if (msg_type == LINHA_TYPE){
            respondLinha(sock, &sockad, &seq, msg_data);
            seq = (seq+1) % MAX_SEQ;
        }
        else if (msg_type == LINHAS_TYPE){
            respondLinhas(sock, &sockad, &seq, msg_data);
            seq = (seq+1) % MAX_SEQ;
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
