cmake_minimum_required(VERSION 3.8)

project("AVStudy"
        VERSION 1.0
        LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(FFMPEG_DIR ${PROJECT_SOURCE_DIR}/3rdparty/ffmpeg)
set(PROJECT_PATH "${PROJECT_SOURCE_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

configure_file(
        "${PROJECT_SOURCE_DIR}/utils/constant.hpp.in"
        "${PROJECT_BINARY_DIR}/utils/constant.hpp"
)

add_subdirectory(demux)
add_subdirectory(decode)
add_subdirectory(encode)