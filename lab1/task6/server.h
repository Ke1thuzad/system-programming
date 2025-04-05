#ifndef SYSTEM_PROGRAMMING_SERVER_H
#define SYSTEM_PROGRAMMING_SERVER_H

#include <error_handler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_MSG_SIZE 4088

typedef struct file_list {
    char *file;
    struct file_list *next;
} FileList;

typedef struct directory {
    char *path;
    FileList *files;
    struct directory *next;
} Directory;

typedef struct request_msg {
    long mtype;
    char data[MAX_MSG_SIZE];
} RequestMsg;

typedef struct response_msg {
    long mtype;
    char data[MAX_MSG_SIZE];
} ResponseMsg;

#endif //SYSTEM_PROGRAMMING_SERVER_H
