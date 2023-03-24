cmake_minimum_required(VERSION 3.15)

project(skye_benchmark)

find_package(skye)
find_package(Boost)

add_executable(
    skye_benchmark
    main.cpp
    mux.cpp)

target_compile_features(skye_benchmark PRIVATE cxx_std_20)

target_link_libraries(
    skye_benchmark
    PRIVATE
    skye::skye
    Boost::boost)

install(TARGETS skye_benchmark RUNTIME)
