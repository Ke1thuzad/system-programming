#ifndef SYSTEM_PROGRAMMING_SERVER_H
#define SYSTEM_PROGRAMMING_SERVER_H

#include <stdio.h>
#include <error_handler.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#include <signal.h>
#include <bits/types/sig_atomic_t.h>

#define PORT 28015

typedef struct msg_buffer {
    long msgtype;
    u_char command;
    u_char argument;
    int client_fd;
} msg_buffer;

typedef enum Command {
    AUTH,
    TAKE,
    PUT,
    MOVE
} Command;

typedef enum Item {
    EMPTY,
    WOLF,
    GOAT,
    CABBAGE
} Item;

typedef struct Game {
    int bank;
    Item current_item;
    int bank_inventories[2][3];
} Game;

void handle_signal();
int check_conflicts(const int *inventory);
void put_item(int *inventory, Item *item);
int process_command(msg_buffer message, Game *game);
void reset_game(Game *game);

#endif //SYSTEM_PROGRAMMING_SERVER_H
