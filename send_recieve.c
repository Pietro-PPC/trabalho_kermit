#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <pthread.h>

#include "send_recieve.h"
#include "message.h"

pthread_mutex_t calculating = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t done = PTHREAD_COND_INITIALIZER;

int sendMessage(int sock, char *msg, int msg_size, struct sockaddr_ll *sockad){
    if (LOG) notifySend(msg);
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
            if (LOG) {printf("sendinsist "); notifyRecieve(response);}
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
    Igual ao sendMessageInsist, mas detecta se houve timeout;
*/
void *sendMessageInsistTimeout(void *data){

    struct sendInsistParams *params = data;
    int ret;
    unsigned char msg_dst, msg_size, msg_sequence, msg_type, msg_data[MAX_DATA_SIZE], msg_parity;
    int oldtype;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

    do { // Tenta enviar mensagem até conseguir
        if (!sendMessage(params->sock, params->msg, MAX_MSG_SIZE, NULL)){
            getNextMessage(params->sock, params->response, params->addr, params->seq, 0);
            if (LOG) {printf("sendinsist "); notifyRecieve(params->response);}
            // Não sei se precisa mandar nack caso ret seja 2
            ret = parseMsg(params->response, &msg_dst, &msg_size, &msg_sequence, &msg_type, msg_data, &msg_parity);
            if (ret == 2) {
                fprintf(stderr, "Fatal error! Response message corrupted!\n");
                exit(1);
            }
        } 
        else
            msg_type = NACK_TYPE; // Parte obsoleta
        
    } while(msg_type == NACK_TYPE);

    pthread_cond_signal(&done);
    return NULL;
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
    Usada para receber algo e mandar um nack até dar certo.
*/
void getMessageInsist(int sock, unsigned char *msg, unsigned char src, unsigned char dest, unsigned char seq){
    unsigned char response[MAX_MSG_SIZE];
    buildNack(response, dest, src, seq);
    while (getNextMessage(sock, msg, dest, seq, 1)){
        if (LOG) notifySend(response);
        sendMessage(sock, response, MAX_MSG_SIZE, NULL);
    }
    if (LOG) {printf("getinsist ");notifyRecieve(msg);}
}

/*
    Usada para receber algo e mandar um nack até dar certo 
    e detectar timeout. 
*/
void *getMessageInsistTimeout(void *data){
    struct getInsistParams *params = data;
    int oldtype;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

    unsigned char response[MAX_MSG_SIZE];
    buildNack(response, params->dest, params->src, params->seq);
    while (getNextMessage(params->sock, params->msg, params->dest, params->seq, 1)){
        if (LOG) notifySend(response);
        sendMessage(params->sock, response, MAX_MSG_SIZE, NULL);
    }
    if (LOG) {printf("getinsist ");notifyRecieve(params->msg);}

    pthread_cond_signal(&done);
    return NULL;
}

/*
    Retornos:
        0: Transmissão finalizada
        1: Transmissão não finalizada
*/
int getMultipleMsgss(int sock, msg_stream_t *s, unsigned char src, unsigned char dest, unsigned char *seq){
    int *i = &(s->size), ret = 0;
    unsigned char type, buf[MAX_MSG_SIZE], response[MAX_MSG_SIZE];
    do{
        getMessageInsist(sock, buf, src, dest, *seq);

        buildAck(response, dest, src, nextSeq(*seq));
        sendMessage(sock, response, MAX_MSG_SIZE, NULL);
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

void *getMultipleMsgssTimeout(void *data){
    struct getMultipleParams *params = data;
    int *i = &(params->s->size);
    unsigned char type, buf[MAX_MSG_SIZE], response[MAX_MSG_SIZE];
    int oldtype;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

    *(params->ret) = 0;
    do{
        getMessageInsist(params->sock, buf, params->src, params->dest, *(params->seq));

        buildAck(response, params->dest, params->src, nextSeq(*(params->seq)));
        sendMessage(params->sock, response, MAX_MSG_SIZE, NULL);
        pushMessage(params->s, buf);
        *(params->seq) = nextSeq(*(params->seq));
        type = getMsgType(buf);

    } while(type != END_TRANSM_TYPE && *i < MAX_STREAM_LEN);
    if (type == END_TRANSM_TYPE) 
        rmLastMessage(params->s);
    else 
        *(params->ret) = 1;
    
    pthread_cond_signal(&done);
    return NULL;
}

void sendMultipleMsgs(int sock, msg_stream_t *msgStream, unsigned char myaddr, unsigned char *seq){
    unsigned char response[MAX_MSG_SIZE];
    for (int i = 0; i < msgStream->size; i++){
        *seq = nextSeq(*seq);
        sendMessageInsist(sock, msgStream->stream[i], NULL, response, myaddr, *seq);
    }
}

/*
    Retornos:
        . 0: t1 < t2
        . Não 0: t1 >= t2
*/
int menorOp(struct timespec *t1, struct timespec *t2){
    if (t1->tv_sec == t2->tv_sec)
        return t1->tv_nsec < t2->tv_nsec;
    return t1->tv_sec < t2->tv_sec;
}


int executeOrTimeout(void *(*func_addr)(void *), void *data, struct timespec *max_wait){
    struct timespec max_time, cur_time;
    pthread_t tid;
    int err, ret;
    
    pthread_mutex_lock(&calculating);

    clock_gettime(CLOCK_REALTIME, &cur_time);
    clock_gettime(CLOCK_REALTIME, &max_time);
    max_time.tv_sec += max_wait->tv_sec;
    max_time.tv_nsec += max_wait->tv_nsec;

    pthread_create(&tid, NULL, func_addr, data);

    err = pthread_cond_timedwait(&done, &calculating, &max_time);

    pthread_cond_signal(&done);

    if (err) return -1;
    pthread_mutex_unlock(&calculating);
    return 0;
}
