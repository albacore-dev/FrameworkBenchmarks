# conan generator requires 3.15
cmake_minimum_required(VERSION 3.15)

project(httpmicroservice_benchmark)

include(FetchContent)

FetchContent_Declare(
    httpmicroservice
    GIT_REPOSITORY https://github.com/luketokheim/httpmicroservice-cpp.git
    GIT_TAG 20f8e6fd37e921e1e72ccea790bacb832d3ac682
    OVERRIDE_FIND_PACKAGE)

set(ENABLE_STANDALONE ON)
set(ENABLE_IO_URING ON)
set(BUILD_TESTING OFF)

set(httpmicroservice_ENABLE_STANDALONE ON)
set(httpmicroservice_ENABLE_IO_URING ON)
set(httpmicroservice_BUILD_TESTING OFF)

find_package(httpmicroservice)

add_executable(httpmicroservice_benchmark handler.cpp main.cpp)

target_link_libraries(
    httpmicroservice_benchmark
    PRIVATE
    httpmicroservice::httpmicroservice)

install(TARGETS httpmicroservice_benchmark RUNTIME)
