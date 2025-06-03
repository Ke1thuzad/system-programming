add_library(thread_safe_queue STATIC
        src/ThreadSafeQueue.cpp
)

target_include_directories(thread_safe_queue PUBLIC include)