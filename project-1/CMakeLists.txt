cmake_minimum_required(VERSION 3.24)
project(project_1 C CXX)

set(CMAKE_C_STANDARD 23)

find_library(PCAP
        NAMES pcap
        HINTS "/opt/homebrew/opt/libpcap/lib"
        )

add_library(checksum libs/checksum.c libs/checksum.h)

add_executable(trace trace.c)
target_link_libraries(trace checksum PCAP)

