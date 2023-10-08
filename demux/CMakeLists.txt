cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

add_executable(demux main.cpp
        demux.cpp
        adts.cpp
)

target_include_directories(demux PRIVATE ${FFMPEG_DIR}/include)
target_include_directories(demux PRIVATE ${PROJECT_BINARY_DIR})

target_link_directories(demux PRIVATE ${FFMPEG_DIR}/lib)

target_link_libraries(demux PRIVATE
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        postproc
        swresample
        swscale)