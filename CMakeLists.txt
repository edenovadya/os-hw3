cmake_minimum_required(VERSION 3.27)
project(HW3 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(HW3
    client.c
    log.c
    log.h
    output.c
    request.c
    request.h
    segel.c
    segel.h
    server.c)
