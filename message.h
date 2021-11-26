#ifndef __MESSAGE__
#define __MESSAGE__

#define SERVER_ADD 2 
#define CLIENT_ADD 1

#define MAX_CMD_LEN 10
#define MAX_MSG_SIZE 19

#define CD_STR "cd"

void buildMsg(unsigned char *raw_msg, unsigned char *parsed_msg);

void parseMsg();

#endif