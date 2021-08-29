cmake_minimum_required(VERSION 3.10)
project(mapping_policy C)

set(CMAKE_C_STANDARD 99)

add_executable(mapping_policy
        main.c
        utils.c
        codegen.c)
