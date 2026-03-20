add_executable(benchmarks
    benchmark_compression.cpp
)
include(FetchContent)

FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb
)

FetchContent_MakeAvailable(stb)

target_include_directories(benchmarks PRIVATE ${stb_SOURCE_DIR})
target_link_libraries(benchmarks PRIVATE texture-compressor)
