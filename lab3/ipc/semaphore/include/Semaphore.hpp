#pragma once
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <cerrno>
#include <memory>
#include <vector>
#include <stdexcept>
#include <system_error>

#include "Exceptions.hpp"

class Semaphore {
    key_t _semaphore_key;
    int _semaphore_id;
    bool _is_owner;

    struct SemaphoreDeleter {
        void operator()(const int* id) const {
            if (id && *id != -1) {
                semctl(*id, 0, IPC_RMID);
            }
        }
    };

    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    };
public:
    class Guard {
        const Semaphore& _semaphore;
        unsigned short _sem_index;
        bool _locked;

    public:
        Guard(const Semaphore& sem, unsigned short sem_index, bool acquire_now = true)
                : _semaphore(sem), _sem_index(sem_index), _locked(false) {
            if (acquire_now) lock();
        }

        ~Guard() {
            if (_locked) unlock();
        }

        void lock() {
            if (_locked) return;
            _semaphore.wait_get(_sem_index);
            _locked = true;
        }

        void unlock() {
            if (!_locked) return;
            _semaphore.release(_sem_index);
            _locked = false;
        }

        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
        Guard(Guard&&) = delete;
        Guard& operator=(Guard&&) = delete;
    };

    Semaphore(const char* token_path, int proj_id, int sem_count, int default_value = 1);
    ~Semaphore() = default;

    void wait_get(unsigned short sem_index) const;
    void no_wait_get(unsigned short sem_index) const;
    void release(unsigned short sem_index) const;

    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;
    Semaphore(Semaphore&&) = delete;
    Semaphore& operator=(Semaphore&&) = delete;
};