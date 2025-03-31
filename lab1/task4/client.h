#ifndef SYSTEM_PROGRAMMING_CLIENT_H
#define SYSTEM_PROGRAMMING_CLIENT_H

#include <error_handler.h>
#include <stdio.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 28015
#define MAX_LENGTH 16

typedef struct msg_buffer {
    long msgtype;
    u_char command;
    u_char argument;
    int client_fd;
} msg_buffer;

char* get_command(FILE *file);
int parse_command(char *str, int *cmd, int *argument);

#endif //SYSTEM_PROGRAMMING_CLIENT_H
