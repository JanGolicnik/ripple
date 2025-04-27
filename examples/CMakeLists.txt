cmake_minimum_required(VERSION 3.10)

project(ripple)

set(CMAKE_C_STANDARD 23)

set(SOURCES main.c)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pedantic -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function -Wdouble-promotion -Wno-bitfield-constant-conversion")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(ripple ${SOURCES})

target_include_directories(ripple PUBLIC
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/..
)
