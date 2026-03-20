add_executable(tests
    test_compression.cpp
)
include(FetchContent)

FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb
)

FetchContent_MakeAvailable(stb)

target_include_directories(tests PRIVATE ${stb_SOURCE_DIR})
target_include_directories(tests PRIVATE ${texture-compressor_SOURCE_DIR})
target_link_libraries(tests PRIVATE texture-compressor)


add_test(NAME test_library COMMAND tests)
