add_library(tcp_utils STATIC
        src/TCPCommon.cpp
)

target_include_directories(tcp_utils PUBLIC include)
target_link_libraries(tcp_utils PUBLIC exceptions)
#target_link_libraries()