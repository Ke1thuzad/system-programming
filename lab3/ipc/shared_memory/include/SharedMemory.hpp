#pragma once

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <vector>
#include <span>
#include <cstring>
#include <memory>
#include <functional>

#include "Exceptions.hpp"
#include "Semaphore.hpp"

class SharedMemory {
    key_t _shared_memory_key;
    int _shared_memory_id;
    size_t _size;
    bool _is_owner;

    std::unique_ptr<char, std::function<void(void*)>> _shared_memory_pointer;

    Semaphore _semaphore;
    bool blocked;
    unsigned int _cursor = 0;

public:
    SharedMemory(const char* token_path, int proj_id, size_t size);
    ~SharedMemory();

    void block() {
        _semaphore.wait_get(0);
        blocked = true;
    };
    void unblock() {
        _semaphore.release(0);
        blocked = false;
    }

    void rewrite(std::span<const char> data, unsigned int position_offset = 0);
    void write(std::span<const char> data);

    std::vector<char> read(unsigned int length, unsigned int position_offset = 0);
    std::vector<char> readNext(unsigned int length);

    void clear();

    [[nodiscard]] bool readyToRead(unsigned char flag) const {
        const char* ptr = static_cast<const char*>(_shared_memory_pointer.get());
        return ptr && *ptr == flag;
    }

    [[nodiscard]] bool isEmpty() const {
        return readyToRead(0);
    }

    unsigned int read_cursor = 0;
};