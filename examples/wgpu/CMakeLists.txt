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
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        5d249570393f7a37e037abf22cd6012a4cc56a71
  CMAKE_ARGS
    -DSDL_SHARED=ON
    -DSDL_VIDEO=ON
    -DSDL_STATIC=OFF
    -DSDL_TESTS=OFF
    -DSDL_INSTALL_TESTS=OFF
    -DSDL_EXAMPLES=OFF
    -DSDL_RENDER=OFF
    -DSDL_GPU=OFF
    -DSDL_AUDIO=OFF
    -DSDL_JOYSTICK=OFF
    -DSDL_HAPTIC=OFF
    -DSDL_SENSOR=OFF
    -DSDL_CAMERA=OFF
)

FetchContent_MakeAvailable(SDL2 cglm)

add_subdirectory(./glfw)
add_subdirectory(./webgpu)
add_subdirectory(./sdl2webgpu)
add_subdirectory(./glfw3webgpu)

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
    sdl2webgpu
    cglm
    SDL2::SDL2
)

add_custom_command(TARGET ripple POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:ripple>)

target_copy_webgpu_binaries(${PROJECT_NAME})
