if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Only Linux is supported!")
endif()


add_compile_options(-fsanitize=thread)
add_link_options(-fsanitize=thread)

add_executable(lab1task3 ${LIB_SOURCES} main.h main.c)