cmake_minimum_required(VERSION 3.26)
project(system_programming C)

set(CMAKE_C_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/utility)

file(GLOB LIB_SOURCES "${CMAKE_SOURCE_DIR}/utility/*.c" "${CMAKE_SOURCE_DIR}/utility/arrays/*.c")

add_compile_options(-Wall -Wextra -Werror -fno-omit-frame-pointer -pedantic -Wshadow -Wno-strict-prototypes)

add_subdirectory(lab1)
