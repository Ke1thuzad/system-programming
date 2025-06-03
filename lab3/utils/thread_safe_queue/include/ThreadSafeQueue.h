#ifndef SYSTEM_PROGRAMMING_THREADSAFEQUEUE_H
#define SYSTEM_PROGRAMMING_THREADSAFEQUEUE_H

#include <queue>
#include <pthread.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <cstring>
#include <cstdio>
#include <cerrno>

inline void initialize_mutex_q(pthread_mutex_t *mutex) {
    int ret = pthread_mutex_init(mutex, nullptr);
    if (ret != 0) {
        throw std::runtime_error("Failed to initialize queue mutex: " + std::string(strerror(ret)));
    }
}

inline void destroy_mutex_q(pthread_mutex_t *mutex) {
    int ret = pthread_mutex_destroy(mutex);
    if (ret != 0 && ret != EBUSY) {
        fprintf(stderr, "Warning: Failed to destroy queue mutex: %s\n", strerror(ret));
    }
}

inline void initialize_condition_variable_q(pthread_cond_t *cond) {
    int ret = pthread_cond_init(cond, nullptr);
    if (ret != 0) {
        throw std::runtime_error("Failed to initialize queue condition variable: " + std::string(strerror(ret)));
    }
}

inline void destroy_condition_variable_q(pthread_cond_t *cond) {
    int ret = pthread_cond_destroy(cond);
    if (ret != 0) {
        fprintf(stderr, "Warning: Failed to destroy queue condition variable: %s\n", strerror(ret));
    }
}


template <typename T>
class ThreadSafeQueue {
    private:
    pthread_mutex_t _mutex;
    pthread_cond_t _cond_var;
    std::queue<T> _queue;
    bool _shutdown = false;

    public:
    ThreadSafeQueue() {
        initialize_mutex_q(&_mutex);
        initialize_condition_variable_q(&_cond_var);
    }

    ~ThreadSafeQueue() {
        destroy_condition_variable_q(&_cond_var);
        destroy_mutex_q(&_mutex);
    }


    void Push(T item) {
        if (_shutdown)
            return;

        pthread_mutex_lock(&_mutex);

        _queue.push(std::move(item));
        pthread_mutex_unlock(&_mutex);

        pthread_cond_signal(&_cond_var);
    }

    bool Pop(T& out_value) {
        pthread_mutex_lock(&_mutex);
        while (_queue.empty() && !_shutdown) {
            pthread_cond_wait(&_cond_var, &_mutex);
        }

        if (_shutdown && _queue.empty()) {
            pthread_mutex_unlock(&_mutex);
            return false;
        }

        out_value = std::move(_queue.front());
        _queue.pop();
        pthread_mutex_unlock(&_mutex);
        return true;
    }

    bool IsEmpty() const {
        int ret = pthread_mutex_lock(const_cast<pthread_mutex_t*>(&_mutex));
        if (ret != 0) {
            throw std::runtime_error("Error locking mutex");
        }

        bool is_empty = _queue.empty();
        pthread_mutex_unlock(const_cast<pthread_mutex_t*>(&_mutex));
        return is_empty;
    }

    void Shutdown() {
        pthread_mutex_lock(&_mutex);
        _shutdown = true;
        pthread_mutex_unlock(&_mutex);

        int ret = pthread_cond_broadcast(&_cond_var);
        if (ret != 0) {
            throw std::runtime_error("Error cond failed");
        }
    }

    bool IsShutdown() const {
        int ret = pthread_mutex_lock(const_cast<pthread_mutex_t*>(&_mutex));
        if (ret != 0) {
            throw std::runtime_error("Error locking shutdown mutex");
        }
        bool is_shutdown = _shutdown;
        pthread_mutex_unlock(const_cast<pthread_mutex_t*>(&_mutex));
        return is_shutdown;
    }
};

#endif // SYSTEM_PROGRAMMING_THREADSAFEQUEUE_H
