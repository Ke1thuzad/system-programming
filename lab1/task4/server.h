#ifndef SYSTEM_PROGRAMMING_SERVER_H
#define SYSTEM_PROGRAMMING_SERVER_H

#include <stdio.h>
#include <error_handler.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>

#include <signal.h>
#include <bits/types/sig_atomic_t.h>

#define PORT 28015

typedef enum Item {
    EMPTY,
    WOLF,
    GOAT,
    CABBAGE
} Item;

void handle_signal();
int check_conflicts(const int *inventory);
void put_item(int *inventory, Item *item);

#endif //SYSTEM_PROGRAMMING_SERVER_H
