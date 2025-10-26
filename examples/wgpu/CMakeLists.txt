cmake_minimum_required(VERSION 3.10)
project(ripple)

set(C_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
  cglm
  GIT_REPOSITORY https://github.com/recp/cglm.git
  GIT_TAG        v0.9.5
)

FetchContent_MakeAvailable(cglm)

add_subdirectory(glfw)
add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)

set(SOURCES main.c)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pedantic -Wall -Wextra -Werror -Wno-missing-field-initializers -Wno-unused-parameter -Wno-unused-function -Wno-bitfield-constant-conversion -Wno-misleading-indentation")

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
    cglm
)

target_copy_webgpu_binaries(${PROJECT_NAME})
