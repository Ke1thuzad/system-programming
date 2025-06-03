add_library(tcp_client STATIC
        src/TCPClient.cpp
)
target_include_directories(tcp_client PUBLIC include)
target_link_libraries(tcp_client PUBLIC exceptions tcp_utils)