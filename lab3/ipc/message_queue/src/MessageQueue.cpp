#include "MessageQueue.hpp"

MessageQueue::MessageQueue(const char *token_path, int proj_id) {
    _message_queue_key = ftok(token_path, proj_id);
    if (_message_queue_key == -1) {
        throw IPCException("Token key was not created");
    }

    _message_queue_id = msgget(_message_queue_key, 0);
    if (_message_queue_id == -1) {
        if (errno == ENOENT) {
            _message_queue_id = msgget(_message_queue_key, 0666 | IPC_CREAT | IPC_EXCL);
            if (_message_queue_id == -1) {
                throw IPCException("Message queue was not created or opened");
            }
        }
    }
}

void MessageQueue::send(int64_t user_id, int arg, long type, int flag) const {
    Message message_s {type, user_id, arg};

    if (msgsnd(_message_queue_id, &message_s, sizeof(message_s) - sizeof(type), flag) == -1) {
        throw IPCException("Message queue message was not sent");
    }
}

MessageQueue::Message MessageQueue::receive(long type, int flag) const {
    Message message_s{};

    if (msgrcv(_message_queue_id, &message_s, sizeof(message_s) - sizeof(type), type, flag) == -1) {
        throw IPCException("Message queue message was not received");
    }

    return message_s;
}
