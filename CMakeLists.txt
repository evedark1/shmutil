cmake_minimum_required (VERSION 3.0)
project(shmutil)

set(CMAKE_C_FLAGS "-g -O0 -Wall")

include_directories(include)
aux_source_directory(src shmsrc)
add_library(shmutil STATIC ${shmsrc})

add_subdirectory(example)
add_subdirectory(unittest)