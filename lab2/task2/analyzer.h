#ifndef SYSTEM_PROGRAMMING_ANALYZER_H
#define SYSTEM_PROGRAMMING_ANALYZER_H

#include "thread_safe_queue.h"
#include "ip_stats.h"

#include <string>
#include <pthread.h>

struct AnalyzerThreadData {
    ThreadSafeQueue<std::string>* queue_ptr;
    const volatile bool* shutdown_flag_ptr;
    pthread_mutex_t* shutdown_mutex_ptr;
    AnalyzerData* analyzer_stats_ptr;
    int thread_id;

    AnalyzerThreadData(
            ThreadSafeQueue<std::string>* queue,
            const volatile bool* flag,
            pthread_mutex_t* mutex,
            AnalyzerData* stats,
            int id
    ) : queue_ptr(queue),
        shutdown_flag_ptr(flag),
        shutdown_mutex_ptr(mutex),
        analyzer_stats_ptr(stats),
        thread_id(id)
    {}
};

void* analyzerThread(void* data);

#endif // SYSTEM_PROGRAMMING_ANALYZER_H