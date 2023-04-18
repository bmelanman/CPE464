cmake_minimum_required(VERSION 3.24)
project(project-2 C)

set(CMAKE_C_STANDARD 90)

add_compile_options(-g -Wall -std=c89 -ansi -pedantic)

#find_library(
#        )

#add_library(checksum libs/checksum.c libs/checksum.h)

add_executable(cclient cclient.c cclient.h)
#target_link_libraries(trace checksum PCAP)

