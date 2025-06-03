#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <cerrno>

#include "Exceptions.hpp"

class Semaphore {
    key_t _semaphore_key;
    int _semaphore_id;

    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    };
public:

    Semaphore(const char *token_path, int proj_id, int sem_count, int default_value = 1);

    ~Semaphore() { semctl(_semaphore_id, 0, IPC_RMID); }

    void wait_get(unsigned short sem_index) const;
    void no_wait_get(unsigned short sem_index) const;
    void release(unsigned short sem_index) const;
};
