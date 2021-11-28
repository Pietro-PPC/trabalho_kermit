#ifndef __SEND_RECIEVE__
#define __SEND_RECIEVE__

#include <net/ethernet.h>
#include <linux/if_packet.h>

#define BUF_SIZE 2048

void sendMessage(int sock, char *msg, int msg_size, struct sockaddr_ll *sockad);

int recieveMessage(int sock, unsigned char *buffer, int buf_size, struct sockaddr_ll *packet_info);

#endif