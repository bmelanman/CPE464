cmake_minimum_required(VERSION 3.25)
project(lab07_cpe464 C)

set(CMAKE_C_STANDARD 11)

link_libraries(libcpe464.2.21.a)

add_executable(rcopy rcopy.c rcopy.h)

add_executable(server server.c server.h)
