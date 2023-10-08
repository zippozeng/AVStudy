//
// Created by zippozeng on 2023/10/7.
//

#ifndef AVSTUDY_DEMUXER_HPP
#define AVSTUDY_DEMUXER_HPP

#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/bsf.h>
};

class demuxer {
public:
    demuxer();

    ~demuxer();

    void demux(std::string &filename);

private:
    static void printAudio(AVStream *stream);

    static void printVideo(AVStream *stream);

    static std::string formatTime(double totalTime);

    AVFormatContext *formatCtx = nullptr;
    AVPacket *packet = nullptr;
    const AVBitStreamFilter *avBitStreamFilter = nullptr;
    AVBSFContext *bsfCtx = nullptr;
};


#endif //AVSTUDY_DEMUXER_HPP
