cmake_minimum_required(VERSION 3.23)
project(SOBay C)

set(CMAKE_C_STANDARD 99)

add_executable(frontend
        frontend.h
        frontend.c
        utils.h
        utils.c)

add_executable(backend
        backend.h
        backend.c
        utils.h
        utils.c
        users_lib.h
        users_lib.o)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")