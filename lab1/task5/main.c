#include <unistd.h>
#include "main.h"

int man_queue_toilet() {
    key_t key = ftok("./tokfile", 55);

    int forks_sem_id = semget(key, 3, 0);
    if (forks_sem_id == -1)
        return throw_err(SEMAPHORE_ERROR);


    if (sem_nowait(forks_sem_id, 1) == -1) {
        return 0;
    }

    sem_check(forks_sem_id, 0);

    sem_signal(forks_sem_id, 1);

    sem_nowait(forks_sem_id, 2);

    printf("Man queued up!\n");

    sleep(rand() % 5);

    sem_nowait(forks_sem_id, 1);

    sem_signal(forks_sem_id, 2);

    printf("Man left!\n");

    if (semctl(forks_sem_id, 2, GETVAL) == N) {
        printf("Last man left!\n");
        sem_signal(forks_sem_id, 0);
    }

    sem_signal(forks_sem_id, 1);


    return 1;
}

int woman_queue_toilet() {
    key_t key = ftok("./tokfile", 55);

    int forks_sem_id = semget(key, 3, 0);
    if (forks_sem_id == -1)
        return throw_err(SEMAPHORE_ERROR);


    if (sem_nowait(forks_sem_id, 0) == -1) {
        return 0;
    }

    sem_check(forks_sem_id, 1);

    sem_signal(forks_sem_id, 0);

    sem_nowait(forks_sem_id, 2);

    printf("Woman queued up!\n");

    sleep(rand() % 5);

    sem_nowait(forks_sem_id, 0);

    sem_signal(forks_sem_id, 2);

    printf("Woman left!\n");

    if (semctl(forks_sem_id, 2, GETVAL) == N) {
        printf("Last woman left!\n");
        sem_signal(forks_sem_id, 1);
    }

    sem_signal(forks_sem_id, 0);


    return 1;
}

void* man_thread(void *arg) {
    (void) arg;

    sleep(rand() % 15);

    man_queue_toilet();

    return NULL;
}

void* woman_thread(void *arg) {
    (void) arg;

    sleep(rand() % 15);

    woman_queue_toilet();

    return NULL;
}

int main() {
    key_t key = ftok("./tokfile", 55);

    int forks_sem_id = semget(key, 3, IPC_CREAT | 0666);
    if (forks_sem_id == -1)
        return throw_err(SEMAPHORE_ERROR);

    union semun arg;
    unsigned short init_values[3] = {1, 1, N};
    arg.array = init_values;

    if (semctl(forks_sem_id, 0, SETALL, arg) == -1) {
        semctl(forks_sem_id, 0, IPC_RMID);
        return throw_err(SEMAPHORE_ERROR);
    }

    pthread_t threads[N * 2];

    for (int i = 0; i < N; ++i) {
        pthread_create(&threads[i], NULL, man_thread, NULL);
        pthread_create(&threads[i + N], NULL, woman_thread, NULL);
    }

    for (int i = 0; i < N * 2; ++i) {
        pthread_join(threads[i], NULL);
    }


    semctl(forks_sem_id, 0, IPC_RMID);

    return 0;
}

int sem_nowait(int semid, int sem_num) {
    struct sembuf sb = {sem_num, -1, 0};
    return semop(semid, &sb, 1);
}

int sem_check(int semid, int sem_num) {
    struct sembuf sb = {sem_num, -1, IPC_NOWAIT};
    return semop(semid, &sb, 1);
}

int sem_signal(int semid, int sem_num) {
    struct sembuf sb = {sem_num, 1, 0};
    return semop(semid, &sb, 1);
}
