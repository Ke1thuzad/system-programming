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

void SharedMemory::rewrite(void *data, unsigned int length, unsigned int position_offset) {
    _semaphore.wait_get(0);

    for (unsigned int i = 0; i < length; i++){
        *(_shared_memory_pointer + position_offset + i) = *((char *) data + i);
    }

    _cursor += length;

    _semaphore.release(0);
}

void SharedMemory::write(void *data, unsigned int length) {
    _semaphore.wait_get(0);

    for (unsigned int i = 0; i < length; i++){
        *(_shared_memory_pointer + _cursor + i) = *((char *) data + i);
    }

    _cursor += length;

    _semaphore.release(0);
}

char * SharedMemory::read(unsigned int length, unsigned int position_offset) {
    char *data = new char[length];

    _semaphore.wait_get(0);

    for (unsigned int i = 0; i < length; ++i) {
        *(data + i) = *(_shared_memory_pointer + position_offset + i);
    }

    read_cursor += length;

    _semaphore.release(0);

    return data;
}

void SharedMemory::clear() {
    _semaphore.wait_get(0);

    int i = 0;

    while (*(_shared_memory_pointer + i) != 0) {
        *(_shared_memory_pointer + i) = 0;
        i++;
    }

    _cursor = 0;

    _semaphore.release(0);
}

char *SharedMemory::readNext(unsigned int length) {
    return read(length, read_cursor);
}
