# conan generator requires 3.15
cmake_minimum_required(VERSION 3.15)

project(httpmicroservice_benchmark)

include(FetchContent)

# Build httpmicroservice library
FetchContent_Declare(
    httpmicroservice
    GIT_REPOSITORY https://github.com/luketokheim/httpmicroservice-cpp.git
    GIT_TAG v0.7.0)

set(httpmicroservice_ENABLE_STANDALONE ON)
set(httpmicroservice_ENABLE_IO_URING OFF)
set(httpmicroservice_BUILD_TESTING OFF)

FetchContent_MakeAvailable(httpmicroservice)

add_executable(httpmicroservice_benchmark handler.cpp main.cpp)

target_link_libraries(
    httpmicroservice_benchmark
    PRIVATE
    httpmicroservice::httpmicroservice)

install(TARGETS httpmicroservice_benchmark RUNTIME)

