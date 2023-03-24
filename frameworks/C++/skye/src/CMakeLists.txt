# conan generator requires 3.15
cmake_minimum_required(VERSION 3.15)

project(skye_benchmark)

include(FetchContent)

FetchContent_Declare(
    skye
    GIT_REPOSITORY https://github.com/luketokheim/skye.git
    GIT_TAG 0.9.0
    OVERRIDE_FIND_PACKAGE)

set(ENABLE_STANDALONE ON)
set(ENABLE_IO_URING ON)
set(BUILD_TESTING OFF)

set(skye_ENABLE_STANDALONE ON)
set(skye_ENABLE_IO_URING ON)
set(skye_BUILD_TESTING OFF)

find_package(skye)

add_executable(skye_benchmark handler.cpp main.cpp)

target_link_libraries(
    skye_benchmark
    PRIVATE
    skye::skye)

install(TARGETS skye_benchmark RUNTIME)
