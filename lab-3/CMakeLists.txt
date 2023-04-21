cmake_minimum_required(VERSION 3.25)
project(lab_3 C)

set(CMAKE_C_STANDARD 99)

add_library(serverLibs
        pollLib.c pollLib.h
        )

add_library(networkLibs
        libPDU.c libPDU.h
        safeUtil.c safeUtil.h
        networks.c networks.h
        gethostbyname.c gethostbyname.h
        )

add_executable(cclient cclient.c)
target_link_libraries(cclient networkLibs)

add_executable(server server.c server.h)
target_link_libraries(server networkLibs serverLibs)
