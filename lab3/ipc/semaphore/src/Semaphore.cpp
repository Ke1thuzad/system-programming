#include "Semaphore.hpp"

Semaphore::Semaphore(const char *token_path, int proj_id, int sem_count, int default_value) {
    _semaphore_key = ftok(token_path, proj_id);
    if (_semaphore_key == -1) {
        throw IPCException("Token key was not created");
    }

    _semaphore_id = semget(_semaphore_key, sem_count, 0);
    if (_semaphore_id == -1) {
        if (errno == ENOENT) {
            _semaphore_id = semget(_semaphore_key, sem_count, 0666 | IPC_CREAT | IPC_EXCL);
            if (_semaphore_id == -1) {
                throw IPCException("Semaphore was not created or opened");
            }
        }
    }

    union semun arg{};
    arg.val = default_value;

    if (semctl(_semaphore_id, 0, SETALL, arg) == -1) {
        throw IPCException("Semaphores were not set to default value");
    }
}

void Semaphore::wait_get(unsigned short sem_index) const {
    struct sembuf operation = {sem_index, -1, 0};

    if (semop(_semaphore_id, &operation, 1) == -1) {
        throw IPCException("Semaphore wait_get operation was not completed");
    }
}

void Semaphore::no_wait_get(unsigned short sem_index) const {
    struct sembuf operation = {sem_index, -1, IPC_NOWAIT};

    if (semop(_semaphore_id, &operation, 1) == -1) {
        if (errno == EAGAIN)
            throw IPCException("Semaphore is occupied");

        throw IPCException("Semaphore no_wait_get operation was not completed");
    }
}

void Semaphore::release(unsigned short sem_index) const {
    struct sembuf operation = {sem_index, 1, 0};

    if (semop(_semaphore_id, &operation, 1) == -1) {
        throw IPCException("Semaphore release operation was not completed");
    }
}
