#ifndef SYSTEM_PROGRAMMING_GENERATOR_H
#define SYSTEM_PROGRAMMING_GENERATOR_H

#include "thread_safe_queue.h"
#include "../task1/logger.h"

#include <string>
#include <vector>
#include <random>
#include <pthread.h>
#include <arpa/inet.h>
#include <memory>

struct tcp_traffic_pkg {
    in_port_t src_port;
    in_addr_t dst_addr;
    in_port_t dst_port;
    size_t sz;

    tcp_traffic_pkg(in_port_t sp, in_addr_t da, in_port_t dp, size_t size)
            : src_port(sp), dst_addr(da), dst_port(dp), sz(size) {}
};

struct tcp_traffic_session {
    in_addr_t src_addr;
    std::vector<tcp_traffic_pkg> pkgs;

    explicit tcp_traffic_session(in_addr_t sa) : src_addr(sa) {}
};

struct GeneratorThreadData {
    ThreadSafeQueue<std::string>* queue_ptr;
    const volatile bool* shutdown_flag_ptr;
    pthread_mutex_t* shutdown_mutex_ptr;
    int thread_id;
    LogLevels log_level;

    GeneratorThreadData(
            ThreadSafeQueue<std::string>* queue,
            const volatile bool* flag,
            pthread_mutex_t* mutex,
            int id,
            LogLevels level
    ) : queue_ptr(queue),
        shutdown_flag_ptr(flag),
        shutdown_mutex_ptr(mutex),
        thread_id(id),
        log_level(level)
    {}
};

void* generatorThread(void* data);

#endif // SYSTEM_PROGRAMMING_GENERATOR_H