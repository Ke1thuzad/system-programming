#ifndef SYSTEM_PROGRAMMING_CLIENT_H
#define SYSTEM_PROGRAMMING_MAIN_H

#include <error_handler.h>
#include <stdio.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#define N 50

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int sem_nowait(int semid, int sem_num);
int sem_check(int semid, int sem_num);
int sem_signal(int semid, int sem_num);

#endif //SYSTEM_PROGRAMMING_CLIENT_H
