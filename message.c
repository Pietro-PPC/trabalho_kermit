#include <stdio.h>
#include <string.h>

#include "message.h"

/***********
  FUNÇÕES ÚTEIS
***********/
void initializeMsg(unsigned char *msg){
    memset(msg, 0, MAX_MSG_SIZE * sizeof(unsigned char));
    msg[0] = START_MARKER;
}

void setSrcDst(unsigned char *msg, unsigned char src, unsigned char dst){
    msg[1] += dst << 6;
    msg[1] += src << 4;
}

void setSize(unsigned char *msg, unsigned char size){
    msg[1] += size;
}

void setSeq(unsigned char *msg, unsigned char seq){
    msg[2] += (seq << 4);
}

void setType(unsigned char *msg, unsigned char type){
    msg[2] += type;
}

void setData(unsigned char *msg, unsigned char* data){
    strncpy(msg+3, data, strlen(data));
}

void setParity(unsigned char *msg, unsigned char parity){
    unsigned char msg_size = msg[1] & 0x0F;
    msg[3+msg_size] = parity;
}


unsigned char calcParity(unsigned char *parsed_msg){
    unsigned char msg_size = parsed_msg[1] & 0x0F;
    unsigned char parity = msg_size;

    // faz XOR com o byte seq/tipo e toda a parte de dados
    for (int i = 0; i <= (int)msg_size; ++i){
        parity ^= *(parsed_msg + (2+i)); // 2 é a posição do byte seq/tipo. Depois vêm dados.
    }

    return parity;
}

/*******************
 ENVIO DE MENSAGENS
*******************/

void buildAck(unsigned char *parsed_msg, unsigned char src, unsigned char dst, unsigned char seq){
    unsigned char parity;
    
    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, src, dst);
    setSeq(parsed_msg, seq);
    setType(parsed_msg, ACK_TYPE);
    
    parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);
}

void buildNack(unsigned char *parsed_msg, unsigned char src, unsigned char dst, unsigned char seq){
    unsigned char parity;
    
    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, src, dst);
    setSeq(parsed_msg, seq);
    setType(parsed_msg, NACK_TYPE);
    
    parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);
}

void buildError(unsigned char *parsed_msg, unsigned char error, unsigned char seq){
    unsigned char parity;

    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, SERVER_ADD, CLIENT_ADD);
    setSize(parsed_msg, 1);
    setSeq(parsed_msg, seq);
    setType(parsed_msg, ERROR_TYPE);

    char error_str[2];
    error_str[0] = error;
    error_str[1] = '\0';
    setData(parsed_msg, error_str);

    parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);
}

void buildCd(unsigned char *raw_msg, unsigned char *parsed_msg){
    unsigned char dir[16];
    unsigned char msg_size;

    sscanf(raw_msg + strlen(CD_STR)+1, "%s", dir);
    msg_size = (unsigned char) strlen(dir); 
    setSize(parsed_msg, msg_size); // se for maior que 15 vai dar errado

    setType(parsed_msg, CD_TYPE);
    setData(parsed_msg, dir);

}

void buildLs(unsigned char *raw_msg, unsigned char *parsed_msg){
    setType(parsed_msg, LS_TYPE);
}
/*
  Retornos: 
    . 0 - Mensagem construída com sucesso
    . 1 - Fracasso ao construir mensagem
*/
int buildMsg(unsigned char *raw_msg, unsigned char *parsed_msg, unsigned char seq){
    unsigned char command[MAX_CMD_LEN];
    unsigned char parity;

    sscanf(raw_msg, "%s", command);

    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, CLIENT_ADD, SERVER_ADD);
    setSeq(parsed_msg, seq);

    if (!strcmp(command, CD_STR)){
        buildCd(raw_msg, parsed_msg);
    }
    else if (!strcmp(command, LS_STR)){ // TODO: Talvez colocar um warning caso ls tenha argumentos 
        buildLs(raw_msg, parsed_msg);
    }
    else 
        return 1;
        
    parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);
    return 0;
}

/*************************
 RECEBIMENTO DE MENSAGENS
*************************/

/*
    Retornos de erro
        . 0: Tudo certo;
        . 1: Mensagem não é kermit
        . 2: Mensagem corrompida
*/
int parseMsg(unsigned char *msg, unsigned char *msg_dst, unsigned char *msg_size, 
              unsigned char *msg_sequence, unsigned char *msg_type, unsigned char *msg_data,
              unsigned char *msg_parity){
    if (msg[0] != START_MARKER){
        return 1;
    }
    *msg_dst = (msg[1] & 0xC0) >> 6;
    *msg_size = msg[1] & 0x0F;
    *msg_sequence = (msg[2] & 0xF0) >> 4;
    *msg_type = msg[2] & 0x0F;
    strncpy(msg_data, msg+3, *msg_size);
    msg_data[*msg_size] = '\0';
    *msg_parity = msg[3+*msg_size];

    if ( calcParity(msg) ^ *msg_parity ){
        return 2;
    }
    return 0;
}
