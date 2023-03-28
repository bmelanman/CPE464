cmake_minimum_required(VERSION 3.24)
project(project_1 C)

set(CMAKE_C_STANDARD 23)

add_executable(project_1 trace.c)
add_library(checksum libs/checksum.c libs/checksum.h)
target_link_libraries(project_1 checksum)
