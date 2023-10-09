//
// Created by zippozeng on 2023/10/8.
//

#ifndef AVSTUDY_DECODER_HPP
#define AVSTUDY_DECODER_HPP

#include <string>
#include <fstream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
};

class Decoder {
public:
    Decoder();

    ~Decoder();

    void decode(std::string &filename);

private:
    std::ofstream audioOutputFile, videoOutputFile;
    AVFormatContext *formatCtx = nullptr;
    AVCodecContext *audioCodecCtx = nullptr;
    AVCodecContext *videoCodecCtx = nullptr;
    AVPacket *packet = nullptr;
    AVFrame *avFrame = nullptr;
    // audio
    SwrContext *swrContext = nullptr;
    int sampleRate{}, channels{};
    enum AVSampleFormat sampleFormat;
    uint8_t *audioDstData[1] = {nullptr};
    int audioLineSize{};
    int audioFrameCount{};

    // video
    SwsContext *swsContext = nullptr;
    int width{}, height{};
    enum AVPixelFormat pix_fmt;
    uint8_t *videoDstData[4] = {nullptr, nullptr, nullptr, nullptr};
    int videoDstLineSize[4]{0, 0, 0, 0};
    int videoDstBufSize = 0;
    int videoFrameCount{};


    int initFile();

    int decode(AVCodecContext *codecCtx, AVPacket *pkt, AVFrame *frame);

    int outputAudioFrame(AVFrame *frame);

    int outputVideoFrame(AVFrame *frame);

};


#endif //AVSTUDY_DECODER_HPP
