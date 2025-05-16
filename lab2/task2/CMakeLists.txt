add_executable(lab2task2 generator.h ../task1/logger.cpp ../task1/logger.h
        thread_safe_queue.cpp
        thread_safe_queue.h
        ip_stats.h
        analyzer.cpp
        analyzer.h
        main.cpp
        utils.h
        )

add_link_options(-pthread)