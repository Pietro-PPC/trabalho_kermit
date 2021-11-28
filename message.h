#ifndef __MESSAGE__
#define __MESSAGE__

#define SERVER_ADD 2
#define CLIENT_ADD 1

#define MAX_CMD_LEN 10
#define MAX_MSG_SIZE 19

#define CD_STR "cd"
#define LS_STR "ls"

#define CD_TYPE 0x00
#define LS_TYPE 0x01
#define VER_TYPE 0x02
#define LINHA_TYPE 0x03
#define LINHAS_TYPE 0x04
#define EDIT_TYPE 0x05
#define COMPILAR_TYPE 0x06
#define ACK_TYPE 0x08
#define NACK_TYPE 0x09
#define LINE_LIMITS_TYPE 0x0A
#define LS_CONT_TYPE 0x0B
#define FILE_CONT_TYPE 0x0C
#define END_TRANSM_TYPE 0x0D
#define ERROR_TYPE 0x0F

int buildMsg(unsigned char *raw_msg, unsigned char *parsed_msg);

void parseMsg(unsigned char *msg, unsigned char *msg_size, unsigned char *msg_sequence, 
              unsigned char *msg_type, unsigned char *msg_parity);

#endif