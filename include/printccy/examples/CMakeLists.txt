cmake_minimum_required(VERSION 3.10)

project(printccy_examples)

set(CMAKE_C_STANDARD 11)

set(SOURCES main.c)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function -Wno-bitfield-constant-conversion")

add_executable(main ${SOURCES})