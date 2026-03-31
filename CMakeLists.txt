cmake_minimum_required(VERSION 3.7)
project(texture-compressor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(texture-compressor
    src/Compression.cpp
    src/Decompression.cpp
    src/BC1.cpp
    src/BC4.cpp
    src/Utils.cpp
)

target_include_directories(
    texture-compressor
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)


option(TEXTURE_COMPRESSOR_BUILD_TESTS "Build tests" ON)
option(TEXTURE_COMPRESSOR_BUILD_BENCHMARKS "Build benchmarks" ON)


if(TEXTURE_COMPRESSOR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()


if(TEXTURE_COMPRESSOR_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
