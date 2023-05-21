cmake_minimum_required(VERSION 3.25)
project(project03_cpe464 C)

set(CMAKE_C_STANDARD 11)

# Various supporting libraries
add_library(getHostByName gethostbyname.c gethostbyname.h)
add_library(networkUtils networkUtils.c networkUtils.h)
add_library(libPoll libPoll.c libPoll.h)
add_library(windowLib windowLib.c windowLib.h)

# Add the libraries to the project
link_libraries(
        getHostByName
        networkUtils
        libPoll
        windowLib
)

# Adding the given custom library
add_definitions(-D__LIBCPE464_)
link_libraries(
        ${PROJECT_SOURCE_DIR}/given_files/libcpe464.2.21.a stdc++ dl
)

# UDP Client
add_executable(rcopy rcopy.c rcopy.h cpe464.h)

# UDP Server
add_executable(server server.c server.h cpe464.h)
