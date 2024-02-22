//
// Created by zippozeng on 2024/2/22.
//

#ifndef AVSTUDY_AUDIOENCODER_HPP
#define AVSTUDY_AUDIOENCODER_HPP

#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
}

class AudioEncoder {

public:
    ~AudioEncoder();

    int encode(std::string &filename, std::string &out_file_name);

private:

    const AVCodec *codec = nullptr;

    AVCodecContext *codec_ctx;

    AVPacket *packet;
    AVFrame *frame;

    /*
     * 这里只支持2通道的转换
    */
    static void f32le_convert_to_fltp(const float *f32le, float *fltp, int nb_samples);

    static int select_sample_rate(const AVCodec *codec);

    static int checkSampleFmt(const AVCodec *codec, enum AVSampleFormat sample_fmt);

    static int checkSampleRate(const AVCodec *codec, int sample_rate);

    static int checkChannelLayout(const AVCodec *codec, AVChannelLayout &channel_layout);

    static void getAdtsHeader(AVCodecContext *ctx, uint8_t *adts_header, int aac_length);

    static int encodeInternal(AVCodecContext *codec_ctx, AVFrame *frame, AVPacket *pkt, std::ofstream &outfile);
};


#endif //AVSTUDY_AUDIOENCODER_HPP
