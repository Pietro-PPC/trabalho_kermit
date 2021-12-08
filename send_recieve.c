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
    int len = send(sock, msg, msg_size, 0);
    if (len < 0){
        fprintf(stderr, "Problem with sendto. Errno: %d\n", errno);
        return 1;
    }
    return 0;
}

/*
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
            getNextMessage(sock, response, addr, seq);
            // Não sei se precisa mandar nack caso ret seja 2
            ret = parseMsg(response, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
            if (ret == 2) {
                fprintf(stderr, "Fatal error! Response message corrupted!\n");
                exit(1);
            }
        } 
        else
            msg_type = NACK_TYPE;
        
    } while(msg_type == NACK_TYPE);
}

/*
    Retornos:
        0: Mensagem correta
        2: Mensagem corrompida
*/
int getNextMessage(int sock, unsigned char *msg, unsigned char dest, unsigned char seq){
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
    } while (ret == 1 || msg_dst != dest || msg_sequence != seq);
    return ret;
}

void getMessageInsist(int sock, unsigned char *msg, unsigned char src, unsigned char dest, unsigned char seq){
    unsigned char response[MAX_MSG_SIZE];
    while (getNextMessage(sock, msg, dest, seq)){
        buildNack(response, src, dest, seq);
        sendMessage(sock, msg, MAX_MSG_SIZE, NULL);
    }
}

void getMultipleMsgss(int sock, struct sockaddr_ll *sockad, msg_stream_t *s, unsigned char *seq, unsigned char dest){
    int i = 0;
    getNextMessage(sock, s->stream[i], dest, *seq);
    while(0);
}
