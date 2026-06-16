cmake_minimum_required(VERSION 3.10)
project(texture-compressor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(texture-compressor
    STATIC
    src/Compression.cpp
    src/Decompression.cpp
    src/BC1.cpp
    src/BC4.cpp
    src/BC5.cpp
    src/Utils.cpp
)

target_include_directories(
    texture-compressor
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)


option(TEXTURE_COMPRESSOR_BUILD_DEMO "Build demo" OFF)
option(TEXTURE_COMPRESSOR_SIMD "Build with SIMD" ON)




if(TEXTURE_COMPRESSOR_BUILD_DEMO)
    add_subdirectory(demo)
endif()



if(MSVC)
  target_compile_options(texture-compressor PRIVATE
    $<$<CONFIG:Release>:/O2>
    $<$<CONFIG:RelWithDebInfo>:/O2>
  )
else()
  target_compile_options(texture-compressor PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:RelWithDebInfo>:-O3>
  )
endif()



if(TEXTURE_COMPRESSOR_SIMD)
  target_sources(texture-compressor PRIVATE src/BC1_comp_avx.cpp)

  if(MSVC)
    target_compile_options(texture-compressor PRIVATE
      $<$<CONFIG:Release>:/arch:AVX2>
      $<$<CONFIG:RelWithDebInfo>:/arch:AVX2>
  )
  else()
    target_compile_options(texture-compressor PRIVATE
      $<$<CONFIG:Release>:-march=native>
      $<$<CONFIG:RelWithDebInfo>:-march=native>
    )
  endif()

else()
  target_sources(texture-compressor PRIVATE src/BC1_comp_base.cpp)
endif()
