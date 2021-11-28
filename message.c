#include <stdio.h>
#include <string.h>

#include "message.h"

/***********
  FUNÇÕES ÚTEIS
***********/
void setClientToServer(unsigned char *msg){
    unsigned char buf = 0;
    buf = SERVER_ADD;
    buf <<= 2;
    buf += CLIENT_ADD;
    buf <<= 4;
    msg[1] += buf;
}

unsigned char getParity(unsigned char *parsed_msg){
    unsigned char msg_size = parsed_msg[1] & 0x0F;
    unsigned char parity = msg_size;

    // faz XOR com o byte seq/tipo e toda a parte de dados
    for (int i = 0; i <= (int)msg_size; ++i){
        parity ^= *(parsed_msg + (2+i)); // 2 é a posição do byte seq/tipo. Depois vêm dados.
    }

    return parity;
}

void setParity(unsigned char *parsed_msg){
    unsigned char msg_size = parsed_msg[1] & 0x0F;
    parsed_msg[3+msg_size] = getParity(parsed_msg);
}

/*******************
 ENVIO DE MENSAGENS
*******************/

// falta seq e acho que seria legal modularizar mais ainda
void buildCd(unsigned char *raw_msg, unsigned char *parsed_msg){
    
    setClientToServer(parsed_msg);

    unsigned char dir[16];
    sscanf(raw_msg + strlen(CD_STR)+1, "%s", dir);
    unsigned char msg_size = (unsigned char) strlen(dir); 
    parsed_msg[1] += msg_size; // se for maior que 15 vai dar errado

    parsed_msg[2] += CD_TYPE;
    // copia nome do diretório para a mensagem
    strncpy(parsed_msg+3, dir, strlen(dir));

    setParity(parsed_msg);
}

void buildLs(unsigned char *raw_msg, unsigned char *parsed_msg){
    setClientToServer(parsed_msg);

    parsed_msg[2] += LS_TYPE;

    setParity(parsed_msg);
}

void buildMsg(unsigned char *raw_msg, unsigned char *parsed_msg){
    unsigned char command[MAX_CMD_LEN];
    sscanf(raw_msg, "%s", command);

    // Garantir que bytes são todos 0
    memset(parsed_msg+1, 0, MAX_MSG_SIZE - 1);

    if (!strcmp(command, CD_STR)){
        buildCd(raw_msg, parsed_msg);
    }
    else if (!strcmp(command, LS_STR)){ // TODO: Talvez colocar um warning caso ls tenha argumentos 
        buildLs(raw_msg, parsed_msg);
    }
}

/*************************
 RECEBIMENTO DE MENSAGENS
*************************/

void parseMsg(unsigned char *msg){
    unsigned char msg_size = msg[1] & 0x0F;
    unsigned char msg_sequence = msg[2] & 0xF0;
    unsigned char msg_type = msg[2] & 0x0F;
    unsigned char msg_parity = msg[3+msg_size];
    if ( getParity(msg) ^ msg_parity ){
        fprintf(stderr, "Erro de envio!"); // depois vai ter que virar um NACK
        return;
    }

    if (msg_type == 0){
        printf("cd\n");
    }
    else if (msg_type == 1){
        printf("ls\n");
    }
    else {
        printf("Message isn't regitered");
    }
}
