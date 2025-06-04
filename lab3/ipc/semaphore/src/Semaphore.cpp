#include "Semaphore.hpp"
#include <cstring>
#include <stdexcept>

Semaphore::Semaphore(const char* token_path, int proj_id, int sem_count, int default_value)
        : _is_owner(false) {

    _semaphore_key = ftok(token_path, proj_id);
    if (_semaphore_key == -1) {
        throw IPCException(std::string("ftok failed: ") + strerror(errno));
    }

    _semaphore_id = semget(_semaphore_key, sem_count, 0);
    if (_semaphore_id == -1) {
        if (errno == ENOENT) {
            _semaphore_id = semget(_semaphore_key, sem_count, 0666 | IPC_CREAT | IPC_EXCL);
            if (_semaphore_id == -1) {
                throw IPCException(std::string("semget create failed: ") + strerror(errno));
            }
            _is_owner = true;
        } else {
            throw IPCException(std::string("semget open failed: ") + strerror(errno));
        }
    }

    std::vector<unsigned short> init_values(sem_count, default_value);
    union semun arg;
    arg.array = init_values.data();

    if (semctl(_semaphore_id, 0, SETALL, arg) == -1) {
        if (_is_owner) {
            semctl(_semaphore_id, 0, IPC_RMID);
        }
        throw IPCException(std::string("semctl SETALL failed: ") + strerror(errno));
    }
}

void Semaphore::wait_get(unsigned short sem_index) const {
    struct sembuf op = {static_cast<unsigned short>(sem_index), -1, 0};

    if (semop(_semaphore_id, &op, 1) == -1) {
        throw IPCException(std::string("semop wait failed: ") + strerror(errno));
    }
}

void Semaphore::no_wait_get(unsigned short sem_index) const {
    struct sembuf op = {static_cast<unsigned short>(sem_index), -1, IPC_NOWAIT};

    if (semop(_semaphore_id, &op, 1) == -1) {
        if (errno == EAGAIN) {
            throw IPCException("Semaphore is locked");
        }
        throw IPCException(std::string("semop nowait failed: ") + strerror(errno));
    }
}

void Semaphore::release(unsigned short sem_index) const {
    struct sembuf op = {static_cast<unsigned short>(sem_index), 1, 0};

    if (semop(_semaphore_id, &op, 1) == -1) {
        throw IPCException(std::string("semop release failed: ") + strerror(errno));
    }
}