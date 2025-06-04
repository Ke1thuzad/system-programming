#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <cerrno>

#include "Exceptions.hpp"

class MessageQueue {
    key_t _message_queue_key;
    int _message_queue_id;
public:
    struct Message {
        long mtype;
        int64_t user_id;
        int arg;
    };

    MessageQueue(const char* token_path, int proj_id);

    ~MessageQueue() { msgctl(_message_queue_id, IPC_RMID, nullptr); }

    void send(int64_t user_id, int arg, long type, int flag = 0) const;

    [[nodiscard]] Message receive(long type = 0, int flag = 0) const;
};