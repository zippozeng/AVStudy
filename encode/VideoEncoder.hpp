//
// Created by zippozeng on 2024/2/23.
//

#ifndef AVSTUDY_VIDEOENCODER_HPP
#define AVSTUDY_VIDEOENCODER_HPP


#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
};

class VideoEncoder {

public:
    ~VideoEncoder();

    int encode(std::string &filename, std::string &out_file_name);

private:
    static int64_t getTime();
    static int encodeInternal(AVCodecContext *codec_ctx, AVFrame *frame, AVPacket *pkt, std::ofstream &outfile);
};


#endif //AVSTUDY_VIDEOENCODER_HPP
