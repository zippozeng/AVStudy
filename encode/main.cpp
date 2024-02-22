#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "AudioEncoder.hpp"
#include <utils/constant.hpp>

void encodePCM2AAC() {
    // ffplay -f f32le -ar 44100 -ac 2 -i 44100_f32le_2.pcm
    std::string inputFilePath;
    inputFilePath.append(PROJECT_PATH);
    inputFilePath.append("/res/44100_f32le_2.pcm");
    std::string outputFilePath = "/aac_audio.aac";
    AudioEncoder encoder;
    encoder.encode(inputFilePath, outputFilePath);
}

int main() {
    // 编码pcm为aac
    encodePCM2AAC();
    return 0;
}