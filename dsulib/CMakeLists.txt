cmake_minimum_required(VERSION 3.22)

project(dsulib)

find_package(fmt REQUIRED)

add_library(
        dsulib
        net/endpoint.cpp
        net/endpoint.hpp
        net/udp_socket.cpp
        dsu_server.cpp
        dsu_server.hpp
        crc.hpp
        logger.cpp
)

target_sources(dsulib PRIVATE logger.hpp)
target_link_libraries(dsulib PRIVATE fmt)