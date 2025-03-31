#include "main.h"

int take_fork(int phil_num, int sem_id) {
    int left = (phil_num + 4) % 5;
    int right = (phil_num + 1) % 5;

    sem_nowait(sem_id, 5);

    if (sem_nowait(sem_id, left) == -1) {
        sem_signal(sem_id, 5);
        return 0;
    }

    if (sem_nowait(sem_id, right) == -1) {
        sem_signal(sem_id, left);
        sem_signal(sem_id, 5);
        return 0;
    }

    sem_signal(sem_id, 5);

    return 1;
}

void put_fork(int phil_num, int sem_id) {
    int left = (phil_num + 4) % 5;
    int right = (phil_num + 1) % 5;

    sem_signal(sem_id, left);
    sem_signal(sem_id, right);
}

void* philosopher(void *num) {
    key_t key = ftok("./tokfile", 55);

    int phil_num = *((int *)num);

    int forks_sem_id = semget(key, 6, 0);
    if (forks_sem_id == -1)
        return NULL;

    for (int i = 0; i < 10; i++) {
        sleep (rand() % 10); // Think

        if (take_fork(phil_num, forks_sem_id)) {
            printf("%d philosopher is eating\n", phil_num);
            sleep (rand() % 10);

            put_fork(phil_num, forks_sem_id);
            printf("%d philosopher has put down forks\n", phil_num);
            unsigned short sems[6];
            union semun arg;

            arg.array = sems;

            semctl(forks_sem_id, 0, GETALL, arg);
            printf("%d %d %d %d %d\n\n", sems[0], sems[1], sems[2], sems[3], sems[4]);
        }
    }

    return NULL;
}

int sem_nowait(int semid, int sem_num) {
    struct sembuf sb = {sem_num, -1, IPC_NOWAIT};
    return semop(semid, &sb, 1);
}

int sem_signal(int semid, int sem_num) {
    struct sembuf sb = {sem_num, 1, 0};
    return semop(semid, &sb, 1);
}

int main() {
    key_t key = ftok("./tokfile", 55);

    int phil_ids[5] = {0, 1, 2, 3, 4};

    int forks_sem_id = semget(key, 6, IPC_CREAT | 0666);
    if (forks_sem_id == -1)
        return throw_err(SEMAPHORE_ERROR);

    union semun arg;
    unsigned short init_values[6] = {1, 1, 1, 1, 1, 1};
    arg.array = init_values;

    if (semctl(forks_sem_id, 0, SETALL, arg) == -1)
        return throw_err(SEMAPHORE_ERROR);


    pthread_t pthreads[5];

    for (int i = 0; i < 5; ++i) {
        pthread_create(&pthreads[i], NULL, philosopher, &phil_ids[i]);
    }

    for (int i = 0; i < 5; ++i) {
        pthread_join(pthreads[i], NULL);
    }


    if (semctl(forks_sem_id, 0, IPC_RMID) == -1)
        return throw_err(SEMAPHORE_ERROR);

    return 0;
}
