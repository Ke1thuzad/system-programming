#include "../include/ThreadSafeQueue.h"

//template<typename T>
//bool ThreadSafeQueue<T>::IsShutdown() const {
//    int ret = pthread_mutex_lock(const_cast<pthread_mutex_t*>(&_mutex));
//    if (ret != 0) {
//        throw std::runtime_error("Error locking shutdown mutex");
//    }
//    bool is_shutdown = _shutdown;
//    pthread_mutex_unlock(const_cast<pthread_mutex_t*>(&_mutex));
//    return is_shutdown;
//}

//template<typename T>
//void ThreadSafeQueue<T>::Shutdown() {
//    pthread_mutex_lock(&_mutex);
//    _shutdown = true;
//    pthread_mutex_unlock(&_mutex);
//
//    int ret = pthread_cond_broadcast(&_cond_var);
//    if (ret != 0) {
//        throw std::runtime_error("Error cond failed");
//    }
//}

//template<typename T>
//bool ThreadSafeQueue<T>::IsEmpty() const {
//    int ret = pthread_mutex_lock(const_cast<pthread_mutex_t*>(&_mutex));
//    if (ret != 0) {
//        throw std::runtime_error("Error locking mutex");
//    }
//
//    bool is_empty = _queue.empty();
//    pthread_mutex_unlock(const_cast<pthread_mutex_t*>(&_mutex));
//    return is_empty;
//}

//template<typename T>
//bool ThreadSafeQueue<T>::Pop(T &out_value) {
//    pthread_mutex_lock(&_mutex);
//    while (_queue.empty() && !_shutdown) {
//        pthread_cond_wait(&_cond_var, &_mutex);
//    }
//
//    if (_shutdown && _queue.empty()) {
//        pthread_mutex_unlock(&_mutex);
//        return false;
//    }
//
//    out_value = std::move(_queue.front());
//    _queue.pop();
//    pthread_mutex_unlock(&_mutex);
//    return true;
//}

//template<typename T>
//void ThreadSafeQueue<T>::Push(T item) {
//    if (_shutdown)
//        return;
//
//    pthread_mutex_lock(&_mutex);
//
//    _queue.push(std::move(item));
//    pthread_mutex_unlock(&_mutex);
//
//    pthread_cond_signal(&_cond_var);
//}
