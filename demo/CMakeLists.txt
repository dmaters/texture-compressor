add_executable(demo
    demo_compression.cpp
)
include(FetchContent)

FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb
)

FetchContent_MakeAvailable(stb)

target_include_directories(demo PRIVATE ${stb_SOURCE_DIR})
target_include_directories(demo PRIVATE ${texture-compressor_SOURCE_DIR})
target_link_libraries(demo PRIVATE texture-compressor)
