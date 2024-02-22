cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

add_executable(encode)

target_sources(encode PRIVATE main.cpp AudioEncoder.cpp)

target_include_directories(encode PRIVATE ${FFMPEG_DIR}/include)
target_include_directories(encode PRIVATE ${PROJECT_BINARY_DIR})

target_link_directories(encode PRIVATE ${FFMPEG_DIR}/lib)

target_link_libraries(encode PRIVATE
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        postproc
        swresample
        swscale)