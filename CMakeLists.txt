cmake_minimum_required(VERSION 3.16)
project(ripple)

add_library(ripple INTERFACE)

target_include_directories(ripple
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
