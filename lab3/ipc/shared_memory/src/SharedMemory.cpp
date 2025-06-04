#include "SharedMemory.hpp"
#include <cstring>
#include <iostream>

SharedMemory::SharedMemory(const char* token_path, int proj_id, size_t size)
        : _size(size), _is_owner(false),
          _shared_memory_pointer(nullptr, [](void* ptr) {
              shmdt(ptr);
          }),
          _semaphore(token_path, proj_id, 1) {

    _shared_memory_key = ftok(token_path, proj_id);
    if (_shared_memory_key == -1) {
        throw IPCException("Token key creation failed");
    }

    _shared_memory_id = shmget(_shared_memory_key, size, 0);
    if (_shared_memory_id == -1) {
        if (errno == ENOENT) {
            _shared_memory_id = shmget(_shared_memory_key, size, 0666 | IPC_CREAT | IPC_EXCL);
            if (_shared_memory_id == -1) {
                throw IPCException("Shared memory creation failed");
            }
            _is_owner = true;
        } else {
            throw IPCException("Shared memory access error");
        }
    }

    char* ptr = static_cast<char *>(shmat(_shared_memory_id, nullptr, 0));
    if (ptr == reinterpret_cast<void*>(-1)) {
        if (_is_owner) {
            shmctl(_shared_memory_id, IPC_RMID, nullptr);
        }
        throw IPCException("Shared memory attachment failed");
    }

    _shared_memory_pointer.reset(ptr);
}

SharedMemory::~SharedMemory() {
    if (_is_owner) {
        shmctl(_shared_memory_id, IPC_RMID, nullptr);
    }
}

void SharedMemory::rewrite(std::span<const char> data, unsigned int position_offset) {
    const unsigned int length = data.size();
    if (position_offset + length > _size) {
        throw SubServerException("Write exceeds shared memory size");
    }

    if (!blocked)
        _semaphore.wait_get(0);

    std::memcpy(_shared_memory_pointer.get() + position_offset, data.data(), length);
    _cursor = position_offset + length;

    if (!blocked)
        _semaphore.release(0);
}

void SharedMemory::write(std::span<const char> data) {
    unsigned int length = data.size();
    if (_cursor + length > _size) {
        throw SubServerException("Write exceeds shared memory size");
    }

    if (!blocked)
        _semaphore.wait_get(0);

    std::memcpy(_shared_memory_pointer.get() + _cursor, data.data(), length);
    _cursor += length;

    if (!blocked)
        _semaphore.release(0);
}

std::vector<char> SharedMemory::read(unsigned int length, unsigned int position_offset) {
    if (position_offset + length > _size) {
        throw SubServerException("Read exceeds shared memory size");
    }

    std::vector<char> data(length);

    if (!blocked)
        _semaphore.wait_get(0);

    std::memcpy(data.data(), _shared_memory_pointer.get() + position_offset, length);
    read_cursor += length;

    if (!blocked)
        _semaphore.release(0);

    return data;
}

std::vector<char> SharedMemory::readNext(unsigned int length) {
    return read(length, read_cursor);
}

void SharedMemory::clear() {
    if (!blocked)
        _semaphore.wait_get(0);

    std::memset(_shared_memory_pointer.get(), 0, _size);
    _cursor = 0;

    if (!blocked)
        _semaphore.release(0);
}