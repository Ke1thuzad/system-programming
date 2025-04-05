#ifndef SYSTEM_PROGRAMMING_CLIENT_H
#define SYSTEM_PROGRAMMING_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <error_handler.h>
#include <sys/stat.h>

#define MAX_MSG_SIZE 4088

typedef struct request_msg {
    long mtype;
    char data[MAX_MSG_SIZE];
} RequestMsg;

typedef struct response_msg {
    long mtype;
    char data[MAX_MSG_SIZE];
} ResponseMsg;

#endif //SYSTEM_PROGRAMMING_CLIENT_H
