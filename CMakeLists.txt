
cmake_minimum_required(VERSION 3.22)

project(lib)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

set(CMAKE_C_STANDARD 99)

file(GLOB_RECURSE PICCOLO_SRC piccolo/*.c piccolo/*.h)

add_library(lib SHARED ${PICCOLO_SRC} src/lib.c)

target_link_libraries(lib ${SDL2_LIBRARIES})
