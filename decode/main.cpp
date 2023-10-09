//
// Created by zippozeng on 2023/10/7.
//
#include <iostream>
#include <string>
#include <utils/constant.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "decoder.hpp"

int main(int argc, char *argv[]) {
    const char *version = av_version_info();
    std::cout << "version:" << version << std::endl;
    std::string filename;
    filename.append(PROJECT_PATH);
    filename.append("/res/douyin_6762052051078843661_yuv420p.mp4");
//    filename.append("/res/douyin_6762052051078843661_yuv422p.mp4");
//    filename.append("/res/douyin_6762052051078843661_mono.mp4");
    Decoder decoder;
    decoder.decode(filename);
}