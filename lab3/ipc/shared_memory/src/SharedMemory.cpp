#include "SharedMemory.hpp"

SharedMemory::SharedMemory(const char *token_path, int proj_id, size_t size) : _semaphore(token_path, proj_id, 1) {
    _shared_memory_key = ftok(token_path, proj_id);
    if (_shared_memory_key == -1) {
        throw IPCException("Token key was not created");
    }

    _shared_memory_id = shmget(_shared_memory_key, size, 0);
    if (_shared_memory_id == -1) {
        if (errno == ENOENT) {
            _shared_memory_id = shmget(_shared_memory_key, size, 0666 | IPC_CREAT | IPC_EXCL);
            if (_shared_memory_id == -1) {
                throw IPCException("Shared Memory was not created or opened");
            }
        }
    }

    _shared_memory_pointer = (char*) shmat(_shared_memory_id, nullptr, 0);
    if (_shared_memory_pointer == (char*) -1) {
        throw IPCException("Shared Memory could not have been mapped");
    }
}

SharedMemory::~SharedMemory() {
    shmdt(_shared_memory_pointer);
    shmctl(_shared_memory_id, IPC_RMID, nullptr);
}

void SharedMemory::write(void *data, unsigned int length, unsigned int position_offset, bool empty) {
    _semaphore.wait_get(0);

    for (unsigned int i = 0; i < length; i++){
        *(_shared_memory_pointer + position_offset + i) = *((char *) data + i);
    }

    _semaphore.release(0);

    _empty = empty;
    _cursor += length;
}

void *SharedMemory::read(unsigned int length, unsigned int position_offset) const {
    char *data = new char[length];

    _semaphore.wait_get(0);

    for (unsigned int i = 0; i < length; ++i) {
        *(data + i) = *(_shared_memory_pointer + position_offset + i);
    }

    _semaphore.release(0);

    return (void *) data;
}

void SharedMemory::clear() {
    _semaphore.wait_get(0);

    for (unsigned int i = 0; i < _cursor; ++i) {
        *(_shared_memory_pointer + i) = 0;
    }

    _semaphore.release(0);

    _cursor = 0;
    _empty = true;
}
