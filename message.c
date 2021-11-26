#include <stdio.h>
#include <string.h>

#include "message.h"

void buildCd(unsigned char *raw_msg, unsigned char *parsed_msg){
    
    parsed_msg[1] = SERVER_ADD;
    parsed_msg[1] <<= 2;
    parsed_msg[1] += CLIENT_ADD;
    parsed_msg[1] <<= 4;

    unsigned char dir[16];
    sscanf(raw_msg + strlen(CD_STR)+1, "%s", dir);
    unsigned char msg_size = (unsigned char) strlen(dir); 
    parsed_msg[1] += msg_size; // se for maior que 15 vai dar errado

}

void parseMsg(unsigned char *raw_msg, unsigned char *parsed_msg){
    unsigned char command[MAX_CMD_LEN];
    sscanf(raw_msg, "%s", command);

    memset(parsed_msg+1, 0, MAX_MSG_SIZE - 1);

    if (!strcmp(command, "cd")){
        buildCd(raw_msg, parsed_msg);
    }
}