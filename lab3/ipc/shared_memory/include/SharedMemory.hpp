#pragma once

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "Exceptions.hpp"
#include "Semaphore.hpp"

class SharedMemory {
    key_t _shared_memory_key;
    int _shared_memory_id;

    char *_shared_memory_pointer;

    Semaphore _semaphore;

    unsigned int _cursor = 0;
public:
    unsigned int read_cursor = 0;

    SharedMemory(const char *token_path, int proj_id, size_t size);

    ~SharedMemory();

    void rewrite(void *data, unsigned int length, unsigned int position_offset = 0);

    void write(void *data, unsigned int length);

    [[nodiscard]] char *read(unsigned int length, unsigned int position_offset = 0);

    [[nodiscard]] char *readNext(unsigned int length);

    void clear();

    [[nodiscard]] bool readyToRead(unsigned char flag) const { return *_shared_memory_pointer == flag; }

    [[nodiscard]] bool isEmpty() const { return readyToRead(0); }
};
