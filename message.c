#include <stdio.h>
#include <string.h>

#include "message.h"

void setClientToServer(unsigned char *B){
    *B = SERVER_ADD;
    *B <<= 2;
    *B += CLIENT_ADD;
    *B <<= 4;
}

void setParity(unsigned char *parsed_msg){
    unsigned char msg_size = parsed_msg[1] & 15;
    unsigned char parity = msg_size;

    // faz XOR com o byte seq/tipo e toda a parte de dados
    for (int i = 0; i <= (int)msg_size; ++i){
        parity ^= *(parsed_msg + (2+i)); // 2 é a posição do byte seq/tipo
    }

    parsed_msg[3+msg_size] = parity;
}

// falta seq 
void buildCd(unsigned char *raw_msg, unsigned char *parsed_msg){
    
    setClientToServer(parsed_msg+1);

    unsigned char dir[16];
    sscanf(raw_msg + strlen(CD_STR)+1, "%s", dir);
    unsigned char msg_size = (unsigned char) strlen(dir); 
    parsed_msg[1] += msg_size; // se for maior que 15 vai dar errado

    // copia nome do diretório para a mensagem
    strncpy(parsed_msg+3, dir, strlen(dir));

    setParity(parsed_msg);
}

void parseMsg(unsigned char *raw_msg, unsigned char *parsed_msg){
    unsigned char command[MAX_CMD_LEN];
    sscanf(raw_msg, "%s", command);

    memset(parsed_msg+1, 0, MAX_MSG_SIZE - 1);

    if (!strcmp(command, CD_STR)){
        buildCd(raw_msg, parsed_msg);
    }
}