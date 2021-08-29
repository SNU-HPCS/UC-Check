cmake_minimum_required(VERSION 3.10)
project(transient_check C)

set(CMAKE_C_STANDARD 99)

add_executable(transient_check
        main.c nanoBench.c)

add_executable(transient_check_never_reach
        main.c nanoBench.c)

set_target_properties(transient_check_never_reach
        PROPERTIES
        COMPILE_DEFINITIONS DEBUG_MODE)