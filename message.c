#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "message.h"
#include "common.h"


/**********************
  FUNÇÕES PARA STREAM
**********************/

void resetMsgStream(msg_stream_t *s){
    s->size = 0;
}

int pushMessage( msg_stream_t *s, unsigned char *msg){
    if (s->size < MAX_STREAM_LEN)
        memcpy(s->stream[ (s->size)++ ], msg, MAX_MSG_SIZE);
    else
        return 1;
    return 0;
}

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
    int field_size = min(strlen(data), MAX_DATA_SIZE);
    strncpy(msg+3, data, field_size);
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

/*
    Retorna o número da linha inicial em mensagem de linhas.
    Caso não seja mensagem desse tipo, retorna -1.
*/
int getFirstLineNum(unsigned char *parsed_msg){
    unsigned char type = parsed_msg[2] & 0x0F;
    int num;

    if (type != LINE_LIMITS_TYPE) return -1;
    memcpy(&num, parsed_msg+3, sizeof(int));
    return num;
}

int getLastLineNum(unsigned char *parsed_msg){
    unsigned char type = parsed_msg[2] & 0x0F;
    int num;

    if (type != LINE_LIMITS_TYPE) return -1;
    memcpy(&num, parsed_msg+3+sizeof(int), sizeof(int));
    return num;
}

unsigned char nextSeq(unsigned char seq){
    return (seq+1) % MAX_SEQ;
}

unsigned char prevSeq(unsigned char seq){
    return (seq-1 + MAX_SEQ) % MAX_SEQ;
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

void buildLsFile(unsigned char *parsed_msg, unsigned char *name, unsigned char seq){
    unsigned char msg_size = (unsigned char) min(strlen(name), MAX_DATA_SIZE);
    unsigned char parity;

    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, SERVER_ADD, CLIENT_ADD);
    setSize(parsed_msg, msg_size);
    setSeq(parsed_msg, seq);
    setType(parsed_msg, LS_CONT_TYPE);
    setData(parsed_msg, name);

    parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);
}


void buildFileContent(unsigned char *parsed_msg, unsigned char *content, unsigned char src, unsigned char dst, unsigned char seq){
    unsigned char size = (unsigned char) min(strlen(content), MAX_DATA_SIZE); // questão de segurança
    unsigned char parity;

    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, src, dst);
    setSize(parsed_msg, size);
    setSeq(parsed_msg, seq);
    setType(parsed_msg, FILE_CONT_TYPE);
    setData(parsed_msg, content);

    parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);
}

void buildEndTransmission(unsigned char *parsed_msg, unsigned char src, unsigned char dst, unsigned char seq){
    unsigned char parity;
    
    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, src, dst);
    setSize(parsed_msg, 0);
    setSeq(parsed_msg, seq);
    setType(parsed_msg, END_TRANSM_TYPE);
    
    parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);
}

void buildCd(unsigned char *raw_msg, unsigned char *parsed_msg, unsigned char seq){
    unsigned char dir[16];
    unsigned char msg_size;

    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, CLIENT_ADD, SERVER_ADD);
    setSeq(parsed_msg, seq);

    sscanf(raw_msg + strlen(CD_STR)+1, "%s", dir);
    msg_size = (unsigned char) min(strlen(dir), MAX_DATA_SIZE); 
    setSize(parsed_msg, msg_size);

    setType(parsed_msg, CD_TYPE);
    setData(parsed_msg, dir);

    unsigned char parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);

}

void buildLs(unsigned char *raw_msg, unsigned char *parsed_msg, unsigned char seq){
    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, CLIENT_ADD, SERVER_ADD);
    setSeq(parsed_msg, seq);
    
    setType(parsed_msg, LS_TYPE);

    unsigned char parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);

}

void buildVer(unsigned char *raw_msg, unsigned char *parsed_msg, unsigned char seq){
    unsigned char file[MAX_DATA_SIZE+1];
    unsigned char msg_size;

    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, CLIENT_ADD, SERVER_ADD);
    setSeq(parsed_msg, seq);

    sscanf(raw_msg + strlen(VER_STR)+1, "%s", file);
    msg_size = (unsigned char) min(strlen(file), MAX_DATA_SIZE);
    setSize(parsed_msg, msg_size);

    setType(parsed_msg, VER_TYPE);
    setData(parsed_msg, file);

    unsigned char parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);

}

void buildLinha(unsigned char *raw_msg, unsigned char *parsed_msg, unsigned char seq){
    unsigned char file[MAX_DATA_SIZE+1], lineNum[MAX_INT_LEN+1];
    unsigned char msg_size;

    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, CLIENT_ADD, SERVER_ADD);
    setSeq(parsed_msg, seq);

    sscanf(raw_msg + strlen(LINHA_STR)+1, "%s %s", lineNum, file);
    msg_size = (unsigned char) min(strlen(file), MAX_DATA_SIZE);
    setSize(parsed_msg, msg_size);

    setType(parsed_msg, LINHA_TYPE);
    setData(parsed_msg, file);

    unsigned char parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);

}

void buildValorLinha(unsigned char *raw_msg, unsigned char *parsed_msg, unsigned char seq, unsigned char *command){
    unsigned char lineNumBuf[MAX_BUF_LEN], lineNum[MAX_DATA_SIZE/2];
    int i;
    
    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, CLIENT_ADD, SERVER_ADD);
    setSeq(parsed_msg, seq);

    sscanf(raw_msg + strlen(command)+1, "%s", lineNumBuf);
    i = atoi(lineNumBuf);
    memcpy(lineNum, &i, sizeof(int));

    setSize(parsed_msg, sizeof(int));
    setType(parsed_msg, LINE_LIMITS_TYPE);
    setData(parsed_msg, lineNum);

    unsigned char parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);

}

void buildLinhas(unsigned char *raw_msg, unsigned char *parsed_msg, unsigned char seq){
    unsigned char file[MAX_DATA_SIZE+1], iniLineNum[MAX_INT_LEN+1], endLineNum[MAX_INT_LEN+1];
    unsigned char msg_size;

    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, CLIENT_ADD, SERVER_ADD);
    setSeq(parsed_msg, seq);

    sscanf(raw_msg + strlen(LINHAS_STR)+1, "%s %s %s", iniLineNum, endLineNum, file);
    msg_size = (unsigned char) min(strlen(file), MAX_DATA_SIZE);
    setSize(parsed_msg, msg_size);

    setType(parsed_msg, LINHAS_TYPE);
    setData(parsed_msg, file);

    unsigned char parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);
}

void buildValorLinhas(unsigned char *raw_msg, unsigned char *parsed_msg, unsigned char seq){
    unsigned char iniLineNumBuf[MAX_BUF_LEN], endLineNumBuf[MAX_BUF_LEN], lineNum[MAX_DATA_SIZE/2];
    int i;

    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, CLIENT_ADD, SERVER_ADD);
    setSeq(parsed_msg, seq);

    sscanf(raw_msg + strlen(LINHA_STR)+1, "%s %s", iniLineNumBuf, endLineNumBuf);
    // Copia primeira linha
    i = atoi(iniLineNumBuf);
    memcpy(lineNum, &i, sizeof(int));
    setData(parsed_msg, lineNum);

    // Copia segunda linha
    i = atoi(endLineNumBuf);
    memcpy(lineNum, &i, sizeof(int));
    setData(parsed_msg + sizeof(int), lineNum);

    setSize(parsed_msg, 2*sizeof(int));
    setType(parsed_msg, LINE_LIMITS_TYPE);

    unsigned char parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);
}

void buildEdit(unsigned char *raw_msg, unsigned char *parsed_msg, int seq){
    unsigned char lineNumBuf[MAX_INT_LEN+1], file[MAX_DATA_SIZE+1];
    unsigned char msg_size;
    
    initializeMsg(parsed_msg);
    setSrcDst(parsed_msg, CLIENT_ADD, SERVER_ADD);
    setSeq(parsed_msg, seq);

    sscanf(raw_msg + strlen(EDIT_STR)+1, "%s %s", lineNumBuf, file);
    msg_size = (unsigned char) min(strlen(file), MAX_DATA_SIZE);
    setSize(parsed_msg, msg_size);

    setType(parsed_msg, EDIT_TYPE);
    setData(parsed_msg, file);

    unsigned char parity = calcParity(parsed_msg);
    setParity(parsed_msg, parity);
}

unsigned char *getTextStart(unsigned char *raw_msg){
    unsigned char *c = raw_msg;

    while (*c != '\"' && *c) 
        *(c++);
    if (!(*c)) return NULL;
    return ++c;
}

void getNextBlock(unsigned char **textPtr, unsigned char *buf, int maxSize){
    int i;
    for (i = 0; i < maxSize && **textPtr && **textPtr != '\"'; ++i){
        buf[i] = *((*textPtr)++);
    }
    buf[i] = 0;
}

/*
  Retornos: 
    . 0 - Mensagem construída com sucesso
    . 1 - Fracasso ao construir mensagem
*/
int buildMsgsFromTxt(unsigned char *raw_msg, msg_stream_t *msgStream, unsigned char seq){
    unsigned char command[MAX_BUF_LEN], parsed_msg[MAX_MSG_SIZE], buf[MAX_DATA_SIZE+1];
    unsigned char parity;
    unsigned char *textPtr;

    sscanf(raw_msg, "%s", command);
    resetMsgStream(msgStream);

    if (!strcmp(command, CD_STR)){
        buildCd(raw_msg, parsed_msg, seq);
        pushMessage(msgStream, parsed_msg);
    }
    else if (!strcmp(command, LS_STR)){ // TODO: Talvez colocar um warning caso ls tenha argumentos 
        buildLs(raw_msg, parsed_msg, seq);
        pushMessage(msgStream, parsed_msg);
    }
    else if (!strcmp(command, VER_STR)){
        buildVer(raw_msg, parsed_msg, seq);
        pushMessage(msgStream, parsed_msg);
    }
    else if (!strcmp(command, LINHA_STR)){
        buildLinha(raw_msg, parsed_msg, seq);
        pushMessage(msgStream, parsed_msg);
        seq = nextSeq(seq);
        buildValorLinha(raw_msg, parsed_msg, seq, LINHA_STR);
        pushMessage(msgStream, parsed_msg);
    }
    else if (!strcmp(command, LINHAS_STR)){
        buildLinhas(raw_msg, parsed_msg, seq);
        pushMessage(msgStream, parsed_msg);
        seq = nextSeq(seq);
        buildValorLinhas(raw_msg, parsed_msg, seq);
        pushMessage(msgStream, parsed_msg);
    }
    else if (!strcmp(command, EDIT_STR)){
        buildEdit(raw_msg, parsed_msg, seq);
        pushMessage(msgStream, parsed_msg);
        seq = nextSeq(seq);
        buildValorLinha(raw_msg, parsed_msg, seq, EDIT_STR);
        pushMessage(msgStream, parsed_msg);
        
        if (!(textPtr = getTextStart(raw_msg)))
            return 1;
        seq = nextSeq(seq);
        while (*textPtr != '\"' && *textPtr != 0){ // Mensagem não deve ser muito longa
            getNextBlock(&textPtr, buf, MAX_DATA_SIZE);
            buildFileContent(parsed_msg, buf, CLIENT_ADD, SERVER_ADD, seq);
            seq = nextSeq(seq);
            pushMessage(msgStream, parsed_msg);
        }
        buildEndTransmission(parsed_msg, CLIENT_ADD, SERVER_ADD, seq);
        pushMessage(msgStream, parsed_msg);
    }
    else 
        return 1;
        
    return 0;
}

/*************************
 RECEBIMENTO DE MENSAGENS
*************************/



unsigned char getMsgSize(unsigned char *msg){
    return msg[1] & 0x0F;
}

unsigned char getMsgSeq(unsigned char *msg){
    return (msg[2] & 0xF0) >> 4;
}

unsigned char getMsgType(unsigned char *msg){
    return msg[2] & 0x0F;
}

void getMsgData(unsigned char *msg, unsigned char *data){
    strncpy(data, msg+3, getMsgSize(msg));
    data[getMsgSize(msg)] = '\0';
}

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
// Pode ser útil ter essas funções
// unsigned char getMsgStart();
// unsigned char getMsgSrc();
// unsigned char getMsgSize();
// unsigned char getMsgSeq();
// unsigned char getMsgType();
// void getMsgData();
// unsigned char getMsgParity();


void notifySend(unsigned char *msg){
    printf("Mandarei %x | %d\n", getMsgType(msg), getMsgSeq(msg)); fflush(stdin);
}

void notifyRecieve(unsigned char *msg){
    printf("Recebi %x | %d\n", getMsgType(msg), getMsgSeq(msg)); fflush(stdin);
}

int rmLastMessage(msg_stream_t *s){
    if (s->size > 0) 
        (s->size)--;
    else 
        return 1;
    return 0;
}
