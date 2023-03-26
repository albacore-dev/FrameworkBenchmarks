cmake_minimum_required(VERSION 3.15)

project(skye_benchmark)

find_package(skye)
find_package(Boost)
find_package(fmt)
find_package(SQLite3)

add_executable(
    skye_benchmark
    context.cpp
    database.cpp
    main.cpp
    mux.cpp)

target_compile_features(skye_benchmark PRIVATE cxx_std_20)

target_link_libraries(
    skye_benchmark
    PRIVATE
    skye::skye
    Boost::boost
    fmt::fmt
    SQLite::SQLite3)

# target_compile_definitions(skye_benchmark PRIVATE BOOST_ASIO_HAS_IO_URING BOOST_ASIO_DISABLE_EPOLL)
# target_link_libraries(skye_benchmark PRIVATE uring)

install(TARGETS skye_benchmark RUNTIME)
