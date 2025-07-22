cmake_minimum_required(VERSION 3.10)
project(ripple)

set(CMAKE_C_STANDARD 23)

add_subdirectory(./webgpu)
add_subdirectory(./glfw)
add_subdirectory(./glfw3webgpu)

set(SOURCES main.c)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pedantic -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function -Wno-bitfield-constant-conversion")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(ripple PUBLIC
    ${CMAKE_SOURCE_DIR}/../../include
    ${CMAKE_SOURCE_DIR}/../..
    ${CMAKE_SOURCE_DIR}/lib/webgpu/include
)

target_link_libraries(ripple
    webgpu
    glfw
    glfw3webgpu
)

target_copy_webgpu_binaries(${PROJECT_NAME})
