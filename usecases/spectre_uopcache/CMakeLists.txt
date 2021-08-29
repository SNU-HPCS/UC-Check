cmake_minimum_required(VERSION 3.10)
project(spectre_uopcache C)

set(CMAKE_C_STANDARD 99)

#set(CMAKE_C_FLAGS "-O2 -g -fno-split-stack")
#set(CMAKE_C_FLAGS "-O0 -g")

add_executable(spectre_uopcache main.c)