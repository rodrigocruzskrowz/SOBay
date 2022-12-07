cmake_minimum_required(VERSION 3.23)
project(SOBay C)

set(CMAKE_C_STANDARD 99)

add_executable(SOBay
        #backend.h
        #backend.c
        frontend.h
        frontend.c
        utils.h
        utils.c)
