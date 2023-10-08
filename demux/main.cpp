//
// Created by zippozeng on 2023/10/7.
//
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "demuxer.hpp"
#include <string>
#include <utils/constant.hpp>

int main(int argc, char *argv[]) {
    const char *version = av_version_info();
    std::cout << "version:" << version << std::endl;
    std::string filename;
    filename.append(PROJECT_PATH);
    filename.append("/res/douyin_6762052051078843661.mp4");
    demuxer demux;
    demux.demux(filename);
}