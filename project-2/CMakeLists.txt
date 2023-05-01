cmake_minimum_required(VERSION 3.24)
project(project-2 C)

set(CMAKE_C_STANDARD 99)

add_compile_options(-g -Wall)

# Libraries
add_library(serverTable serverTable.c serverTable.h)
add_library(networkUtils networkUtils.c networkUtils.h libPoll.c libPoll.h)
link_libraries(networkUtils serverTable)

# Executables
add_executable(cclient cclient.c cclient.h)
target_link_libraries(cclient networkUtils)

add_executable(server server.c server.h)
target_link_libraries(server networkUtils)

add_executable(test test.c)
