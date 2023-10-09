cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

add_executable(decode main.cpp
        decoder.cpp
        decoder.hpp)

target_include_directories(decode PRIVATE ${FFMPEG_DIR}/include)
target_include_directories(decode PRIVATE ${PROJECT_BINARY_DIR})

target_link_directories(decode PRIVATE ${FFMPEG_DIR}/lib)

target_link_libraries(decode PRIVATE
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        postproc
        swresample
        swscale)