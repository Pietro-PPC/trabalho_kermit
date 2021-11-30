#ifndef __SEND_RECIEVE__
#define __SEND_RECIEVE__

#include <net/ethernet.h>
#include <linux/if_packet.h>

#define BUF_SIZE 2048

int sendMessage(int sock, char *msg, int msg_size, struct sockaddr_ll *sockad);

int getNextMessage(int sock, unsigned char *msg, unsigned char dest, unsigned char seq);

void sendMessageInsist(int sock, unsigned char *msg, struct sockaddr_ll *sockad, unsigned char *response, unsigned char addr, unsigned char seq);

#endif