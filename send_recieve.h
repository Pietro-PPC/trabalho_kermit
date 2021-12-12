#ifndef __SEND_RECIEVE__
#define __SEND_RECIEVE__

#include <net/ethernet.h>
#include <linux/if_packet.h>

#include "message.h"

#define BUF_SIZE 2048
#define LOG 1

#define GET_INSIST_CODE 0
#define SEND_INSIST_CODE 1
#define SEND_MULTIPLE_CODE 2

struct getInsistParams {
    int sock;
    unsigned char *msg;
    unsigned char src;
    unsigned char dest;
    unsigned char seq;
};

struct sendInsistParams {
    int sock;
    unsigned char *msg;
    struct sockaddr_ll *sockad;
    unsigned char *response;
    unsigned char addr;
    unsigned char seq;
};

struct getMultipleParams {
    int sock;
    msg_stream_t *s;
    unsigned char src;
    unsigned char dest;
    unsigned char *seq;
    int *ret;
};

int sendMessage(int sock, char *msg, int msg_size, struct sockaddr_ll *sockad);

int getNextMessage(int sock, unsigned char *msg, unsigned char dest, unsigned char seq, int command);

void getMessageInsist(int sock, unsigned char *msg, unsigned char src, unsigned char dest, unsigned char seq);

void sendMessageInsist(int sock, unsigned char *msg, struct sockaddr_ll *sockad, unsigned char *response, unsigned char addr, unsigned char seq);

int getMultipleMsgss(int sock, msg_stream_t *s, unsigned char src, unsigned char dest, unsigned char *seq);

int isANE(unsigned char *msg);

void sendMultipleMsgs(int sock, msg_stream_t *msgStream, unsigned char myaddr, unsigned char *seq);

void *sendMessageInsistTimeout(void *data);

void *getMessageInsistTimeout(void *data);

void *getMultipleMsgssTimeout(void *data);

int executeOrTimeout(void *(*func_addr)(void *), void *data, struct timespec *max_wait);

#endif