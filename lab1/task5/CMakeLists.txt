add_compile_options(-fsanitize=thread)
add_link_options(-fsanitize=thread)

add_executable(lab1task5 ${LIB_SOURCES} main.c main.h)
