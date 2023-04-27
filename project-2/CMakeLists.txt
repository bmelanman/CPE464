cmake_minimum_required(VERSION 3.24)
project(project-2 C)

set(CMAKE_C_STANDARD 99)

add_compile_options(-g -Wall -std=c99 -pedantic)

include_directories(src lib)

# Libraries
add_library(networkUtils lib/networkUtils.c lib/networkUtils.h lib/libPoll.c lib/libPoll.h)
add_library(serverTable lib/serverTable.c lib/serverTable.h)

# Executables
add_executable(cclient src/cclient.c src/cclient.h)
target_link_libraries(cclient networkUtils)

add_executable(server src/server.c src/server.h)
target_link_libraries(server networkUtils serverTable)



