add_library(logger STATIC
        src/Logger.cpp
        ../thread_safe_queue/src/ThreadSafeQueue.cpp
)

target_include_directories(logger PUBLIC include)
target_link_libraries(logger PUBLIC thread_safe_queue)