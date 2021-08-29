cmake_minimum_required(VERSION 3.10)
project(sharing_policy C)

set(CMAKE_C_STANDARD 99)

add_executable(sharing_policy
        main.c
        utils.c
        codegen.c)
