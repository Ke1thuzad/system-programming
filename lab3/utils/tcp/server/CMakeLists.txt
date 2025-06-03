add_library(tcp_server STATIC
        src/TCPServer.cpp
)
target_include_directories(tcp_server PUBLIC include)
target_link_libraries(tcp_server PUBLIC exceptions tcp_utils)