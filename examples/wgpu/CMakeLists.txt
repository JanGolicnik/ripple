cmake_minimum_required(VERSION 3.10)
project(ripple)

set(C_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
  cglm
  GIT_REPOSITORY https://github.com/recp/cglm.git
  GIT_TAG        v0.9.5
)

FetchContent_Declare(
    marrow
    GIT_REPOSITORY https://github.com/JanGolicnik/marrow.git
)

FetchContent_Declare(
    printccy
    GIT_REPOSITORY https://github.com/JanGolicnik/printccy.git
)

FetchContent_MakeAvailable(cglm marrow printccy)

add_subdirectory(glfw)
add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)

set(SOURCES main.c)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pedantic -Wall -Wextra -Werror -Wno-missing-field-initializers -Wno-unused-parameter -Wno-unused-function -Wno-bitfield-constant-conversion -Wno-misleading-indentation")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/../..
    ${CMAKE_SOURCE_DIR}/../../vendor
    ${CMAKE_SOURCE_DIR}/lib/webgpu/include
)

target_link_libraries(${PROJECT_NAME}
    webgpu
    glfw
    glfw3webgpu
    cglm
    marrow
    printccy
)

target_copy_webgpu_binaries(${PROJECT_NAME})
