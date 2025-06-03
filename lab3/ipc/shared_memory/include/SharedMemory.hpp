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

    bool _empty = true;

    unsigned int _cursor = 0;
public:

    SharedMemory(const char *token_path, int proj_id, size_t size);

    ~SharedMemory();

    void write(void *data, unsigned int length, unsigned int position_offset = 0, bool empty = false);

    [[nodiscard]] void *read(unsigned int length, unsigned int position_offset = 0) const;

    void clear();

    bool isEmpty() const { return _empty; }
};
