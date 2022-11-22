
cmake_minimum_required(VERSION 3.23)
project(lasercsv)

add_library(
    lasercsv
    INTERFACE
    )

target_compile_features(
    lasercsv
    INTERFACE
    cxx_std_17
    )

target_include_directories(
    lasercsv
    INTERFACE
    include
    )

find_package(Threads)
target_link_libraries(
    lasercsv
    INTERFACE
    ${CMAKE_THREAD_LIBS_INIT}
    )

add_subdirectory(
    test
    )
