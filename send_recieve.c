#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>

#include "send_recieve.h"
#include "message.h"

int sendMessage(int sock, char *msg, int msg_size, struct sockaddr_ll *sockad){
    // notifySend(msg);
    int len = send(sock, msg, msg_size, 0);
    if (len < 0){
        fprintf(stderr, "Problem with sendto. Errno: %d\n", errno);
        exit(1);
    }
    return 0;
}

/*
    Manda mensagem até receber resposta
    Retornos
        . 0: Recebeu ack
        . 1: Timeout (não implementado)
        . 2: resposta corrompida?
    TODO: remover parametro de seq
    Detectaremos o timeout aqui
*/
void sendMessageInsist(int sock, unsigned char *msg, struct sockaddr_ll *sockad, unsigned char *response, unsigned char addr, unsigned char seq){
    int ret;
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_data[MAX_DATA_SIZE], msg_parity;
    do { // Tenta enviar mensagem até conseguir
        if (!sendMessage(sock, msg, MAX_MSG_SIZE, sockad)){
            getNextMessage(sock, response, addr, seq, 0);
            // printf("sendinsist "); notifyRecieve(response);
            // Não sei se precisa mandar nack caso ret seja 2
            ret = parseMsg(response, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
            if (ret == 2) {
                fprintf(stderr, "Fatal error! Response message corrupted!\n");
                exit(1);
            }
        } 
        else
            msg_type = NACK_TYPE; // Parte obsoleta
        
    } while(msg_type == NACK_TYPE);
}

/*
    Retorna se mensagem é ack, nack ou erro
    Função importante pra saber se incrementa ou não o sequence number
*/
int isANE(unsigned char *msg){
    unsigned char type = getMsgType(msg);
    return (type == ACK_TYPE || type == NACK_TYPE || type == ERROR_TYPE);
}

/*
    Pega próxima mensagem destiada a dest
    Retornos:
        0: Mensagem correta
        2: Mensagem corrompida
*/
int getNextMessage(int sock, unsigned char *msg, unsigned char dest, unsigned char seq, int notANE){
    struct sockaddr_ll packet_info;
    int packet_info_size = sizeof(struct sockaddr_ll);
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_data[MAX_DATA_SIZE], msg_parity;
    int len, ret;
    do {
        len = recv(sock, msg, MAX_MSG_SIZE, 0);
        if (len < 0){
            fprintf(stderr, "Error recieving message!\n");
            ret = 1;
        } else  
            ret = parseMsg(msg, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
    } while (ret == 1 || msg_dst != dest || 
            isANE(msg) && notANE ||
            isANE(msg) && msg_sequence != seq || 
            !isANE(msg) && msg_sequence != nextSeq(seq));
    return ret;
}

/*
    Usada para receber algo e mandar um ack/nack de resposta
*/
void getMessageInsist(int sock, unsigned char *msg, unsigned char src, unsigned char dest, unsigned char seq){
    unsigned char response[MAX_MSG_SIZE];
    buildNack(response, dest, src, seq);
    while (getNextMessage(sock, msg, dest, seq, 1)){
        printf("Recebi Corrompida\n");
        // notifySend(response);
        sendMessage(sock, response, MAX_MSG_SIZE, NULL);
    }
    // printf("getinsist ");notifyRecieve(msg);
    buildAck(response, dest, src, nextSeq(seq));
    sendMessage(sock, response, MAX_MSG_SIZE, NULL);
}

/*
    Retornos:
        0: Transmissão finalizada
        1: Transmissão não finalizada
*/
int getMultipleMsgss(int sock, msg_stream_t *s, unsigned char src, unsigned char dest, unsigned char *seq){
    int *i = &(s->size), ret = 0;
    unsigned char type, buf[MAX_MSG_SIZE];
    do{
        getMessageInsist(sock, buf, src, dest, *seq);
        pushMessage(s, buf);
        *seq = nextSeq(*seq);
        type = getMsgType(buf);

    } while(type != END_TRANSM_TYPE && *i < MAX_STREAM_LEN);
    if (type == END_TRANSM_TYPE) 
        rmLastMessage(s);
    else 
        ret = 1;
    return ret;
}
