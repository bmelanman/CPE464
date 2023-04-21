cmake_minimum_required(VERSION 3.24)
project(project-2 C)

set(CMAKE_C_STANDARD 90)

add_compile_options(-g -Wall -std=c89 -ansi -pedantic)

# Personal util library
add_library(network_utils libs/network_utils.c libs/network_utils.h)

# Given network utils
file(GLOB_RECURSE GIVEN_UTILS "given_files/tcpUdpPollCode/*")
add_library(given_code ${GIVEN_UTILS})

add_library(dict libs/dictionary.c libs/dictionary.h)

add_executable(cclient src/cclient.c src/cclient.h)

add_executable(server src/server.c src/server.h)

add_executable(test src/test.c)

target_link_libraries(server dict)
target_link_libraries(test dict)
