cmake_minimum_required(VERSION 3.26)
project(system_programming CXX)

set(CMAKE_CXX_STANDARD 23)

add_compile_options(-Wall -Wextra -Werror -fno-omit-frame-pointer -pedantic -Wshadow)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address -fsanitize=leak)
    add_link_options(-fsanitize=address -fsanitize=leak)
endif()

add_subdirectory(utils)
add_subdirectory(ipc)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(subprocesses)
