cmake_minimum_required(VERSION 3.10)

project(ripple)

set(CMAKE_C_STANDARD 11)

set(SOURCES main.c)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Werror")

add_executable(ripple ${SOURCES})

target_include_directories(ripple PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
