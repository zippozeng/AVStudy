//
// Created by zippozeng on 2023/10/8.
//

#include "decoder.hpp"
#include <iostream>
#include <fstream>
#include <utils/constant.hpp>

extern "C" {
#include <libavutil/time.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
}

Decoder::Decoder() {
}

Decoder::~Decoder() {
    if (formatCtx != nullptr) {
        avformat_free_context(formatCtx);
        formatCtx = nullptr;
    }
    if (audioCodecCtx != nullptr) {
        avcodec_free_context(&audioCodecCtx);
        audioCodecCtx = nullptr;
    }
    if (videoCodecCtx != nullptr) {
        avcodec_free_context(&videoCodecCtx);
        videoCodecCtx = nullptr;
    }
    if (swrContext != nullptr) {
        swr_free(&swrContext);
        swrContext = nullptr;
    }
    if (swsContext != nullptr) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }
    if (packet != nullptr) {
        av_packet_free(&packet);
        packet = nullptr;
    }
    if (avFrame != nullptr) {
        av_frame_free(&avFrame);
        avFrame = nullptr;
    }
    for (auto &i: videoDstData) {
        if (i != nullptr) {
            delete[] i; // 如果是动态分配的数组内存，使用delete[]
            i = nullptr; // 将指针设为nullptr
        }
    }
}

int Decoder::initFile() {
    std::string directoryName;
    directoryName.append(PROJECT_PATH);
    directoryName.append("/out");
    // 检查文件夹是否存在，如果不存在则创建
    if (!std::filesystem::exists(directoryName)) {
        // 如果文件夹不存在，使用 std::filesystem::create_directory() 来创建它
        if (std::filesystem::create_directory(directoryName)) {
            std::cout << "create directory success:" << directoryName << std::endl;
        } else {
            std::cerr << "create directory failed:" << directoryName << std::endl;
            return -1;
        }
    }

    std::string audioOutFilePath;
    audioOutFilePath.append(directoryName);
    audioOutFilePath.append("/out.pcm");
    std::cout << "audio path:" << audioOutFilePath << std::endl;

    std::string videoOutFilePath;
    videoOutFilePath.append(directoryName);
    videoOutFilePath.append("/out.yuv");
    std::cout << "video path:" << videoOutFilePath << std::endl;

    audioOutputFile.open(audioOutFilePath, std::ios::binary);
    videoOutputFile.open(videoOutFilePath, std::ios::binary);

    if (!audioOutputFile.is_open()) {
        std::cerr << "Can't open file" << audioOutFilePath << std::endl;
        return -1; // 返回错误码
    }
    if (!videoOutputFile.is_open()) {
        std::cerr << "Can't open file:" << videoOutFilePath << std::endl;
        return -1; // 返回错误码
    }
    return 0;
}

void Decoder::decode(std::string &filename) {
    std::cout << "decode file name:" << filename << std::endl;
    initFile();

    formatCtx = avformat_alloc_context();
    // 1. 打开文件
    int ret = avformat_open_input(&formatCtx, filename.c_str(), nullptr, nullptr);
    if (ret) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cout << "open " << filename << " failed:" << buf << std::endl;
        return;
    }

    /* retrieve stream information */
    ret = avformat_find_stream_info(formatCtx, nullptr);
    if (ret) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cout << "Could not find stream information:" << buf << std::endl;
        return;
    }

    // 2. 查找流索引
    int audioTrackID = av_find_best_stream(formatCtx, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioTrackID < 0) {
        char buf[1024] = {0};
        av_strerror(audioTrackID, buf, sizeof(buf) - 1);
        std::cout << "find audio track id failed:" << buf << std::endl;
        return;
    }
    int videoTrackID = av_find_best_stream(formatCtx, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoTrackID < 0) {
        char buf[1024] = {0};
        av_strerror(videoTrackID, buf, sizeof(buf) - 1);
        std::cout << "find video track id failed:" << buf << std::endl;
        return;
    }

    AVStream *audioStream = formatCtx->streams[audioTrackID];
    const AVCodec *audioCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    if (audioCodec == nullptr) {
        std::cout << "find audio decoder failed" << std::endl;
        return;
    }
    audioCodecCtx = avcodec_alloc_context3(audioCodec);
    if (audioCodecCtx == nullptr) {
        std::cout << "alloc audio codec context failed" << std::endl;
        return;
    }
    ret = avcodec_parameters_to_context(audioCodecCtx, audioStream->codecpar);
    if (ret) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cout << "audio codecpar copy to codec context failed:" << buf << std::endl;
        return;
    }
    ret = avcodec_open2(audioCodecCtx, audioCodec, nullptr);
    if (ret < 0) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cout << "audio codec open failed:" << buf << std::endl;
        return;
    }
    // get audio information
    sampleRate = audioCodecCtx->sample_rate;
    channels = audioCodecCtx->ch_layout.nb_channels;
    sampleFormat = audioCodecCtx->sample_fmt;

    if (sampleFormat != AV_SAMPLE_FMT_FLTP) {
        // resample
        ret = swr_alloc_set_opts2(&swrContext, const_cast<const AVChannelLayout *>(&audioCodecCtx->ch_layout),
                                  AV_SAMPLE_FMT_FLTP, sampleRate,
                                  const_cast<const AVChannelLayout *>(&audioCodecCtx->ch_layout),
                                  audioCodecCtx->sample_fmt, sampleRate, 0, nullptr);
        if (ret) {
            std::cerr << "can't create audio SwrContext" << std::endl;
            return;
        }
        ret = swr_init(swrContext);
        if (ret < 0) {
            std::cerr << "Failed to initialize resampler" << std::endl;
            return;
        }
    }
    std::cout << "audio sampleRate=" << sampleRate << " channels:" << channels
              << " sampleFormat:" << av_get_sample_fmt_name(sampleFormat)
              << std::endl;

    AVStream *videoStream = formatCtx->streams[videoTrackID];
    const AVCodec *videoCodec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (videoCodec == nullptr) {
        std::cout << "find video decoder failed" << std::endl;
        return;
    }
    videoCodecCtx = avcodec_alloc_context3(videoCodec);
    if (videoCodecCtx == nullptr) {
        std::cout << "alloc video codec context failed" << std::endl;
        return;
    }
    ret = avcodec_parameters_to_context(videoCodecCtx, videoStream->codecpar);
    if (ret) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cout << "video codecpar copy to codec context failed:" << buf << std::endl;
        return;
    }
    ret = avcodec_open2(videoCodecCtx, videoCodec, nullptr);
    if (ret < 0) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cout << "video codec open failed:" << buf << std::endl;
        return;
    }
    // get video information
    width = videoCodecCtx->width;
    height = videoCodecCtx->height;
    pix_fmt = videoCodecCtx->pix_fmt;
    if (pix_fmt != AV_PIX_FMT_YUV420P) {
        swsContext = sws_getContext(width, height, pix_fmt,
                                    width, height, AV_PIX_FMT_YUV420P,
                                    SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (swsContext == nullptr) {
            std::cerr << "can't create video SwsContext" << std::endl;
            return;
        }
    }
    std::cout << "video width x height=" << width << "x" << height
              << " pix_fmt:" << av_get_pix_fmt_name(pix_fmt) << std::endl;

    ret = av_image_alloc(videoDstData, videoDstLineSize,
                         width, height, pix_fmt, 1);
    if (ret < 0) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cout << "Could not allocate raw video buffer:" << buf << std::endl;
        return;
    }
    videoDstBufSize = ret;
    std::cout << "video videoDstBufSize=" << videoDstBufSize << std::endl;

    packet = av_packet_alloc();
    if (packet == nullptr) {
        std::cout << "alloc packet failed" << std::endl;
        return;
    }
    avFrame = av_frame_alloc();
    if (packet == nullptr) {
        std::cout << "alloc frame buffer failed" << std::endl;
        return;
    }
    while ((ret = av_read_frame(formatCtx, packet)) == 0) {
        if (packet->stream_index == audioTrackID) {
            decode(audioCodecCtx, packet, avFrame);
        } else if (packet->stream_index == videoTrackID) {
            decode(videoCodecCtx, packet, avFrame);
        }
    }
    if (ret) {
        if (ret == AVERROR_EOF) {
            std::cout << "End of file:" << filename << std::endl;
        } else {
            char buf[1024] = {0};
            av_strerror(ret, buf, sizeof(buf) - 1);
            std::cout << "read frame failed:" << ret << " " << buf << std::endl;
            return;
        }
    }
    // flush decode
    packet->data = nullptr;
    packet->size = 0;
    decode(audioCodecCtx, packet, avFrame);

    packet->data = nullptr;
    packet->size = 0;
    decode(videoCodecCtx, packet, avFrame);
}

int Decoder::decode(AVCodecContext *codecCtx, AVPacket *pkt, AVFrame *frame) {
    int ret = avcodec_send_packet(codecCtx, pkt);
    if (ret) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cout << "decode video send packet failed:" << buf << std::endl;
        return ret;
    }
    while (true) {
        ret = avcodec_receive_frame(codecCtx, frame);
        if (ret) {
            if (ret == AVERROR_EOF) {
                std::cout << "decode video Enf of file" << std::endl;
                break;
            } else if (ret == AVERROR(EAGAIN)) {
                break;
            } else {
                char buf[1024] = {0};
                av_strerror(ret, buf, sizeof(buf) - 1);
                std::cout << "decode video receive frame failed:" << buf << std::endl;
                break;
            }
        }
        if (codecCtx->codec->type == AVMEDIA_TYPE_VIDEO) {
            ret = outputVideoFrame(frame);
        } else {
            ret = outputAudioFrame(frame);
        }
        av_frame_unref(frame);
        if (ret < 0) {
            return ret;
        }
    }
    return 0;
}

int Decoder::outputAudioFrame(AVFrame *frame) {
    int numBytes = av_get_bytes_per_sample(sampleFormat);
    std::cout << "audio frame n:" << audioFrameCount++
              << " nb_samples:" << frame->nb_samples
              << "  bytes:" << av_get_bytes_per_sample(sampleFormat)
              << std::endl;

    if (swrContext) {
        // aac只支持AV_SAMPLE_FMT_FLTP格式
        swr_convert(swrContext, reinterpret_cast<uint8_t **>(&audioDstData), 0,
                    const_cast<const uint8_t **>(frame->data), frame->nb_samples);
    }

    for (int i = 0; i < frame->nb_samples; i++) {
        for (int ch = 0; ch < audioCodecCtx->ch_layout.nb_channels; ch++) {
            // planar模式：LLLLRRRR
            // packed方式写入：LRLRLRLR, ffplay默认播放的是
            audioOutputFile.write(reinterpret_cast<const char *>(avFrame->data[ch] + numBytes * i), numBytes);
        }
    }

    return 0;
}

int Decoder::outputVideoFrame(AVFrame *frame) {
    if (frame->width != width || frame->height != height || frame->format != pix_fmt) {
        std::cout << "Error: Width, height and pixel format have to be "
                  << "constant in a rawvideo file, but the width, height or "
                  << "pixel format of the input video changed:" << std::endl
                  << "old: width = " << width << ", height = " << height
                  << ", format = " << av_get_pix_fmt_name(pix_fmt) << std::endl
                  << "new: width = " << frame->width << ", height = " << frame->height
                  << ", format = " << av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format))
                  << std::endl;
        return -1;
    }
    std::cout << "video frame n:" << videoFrameCount++ << std::endl;
    if (swsContext) {
        // If the color space is not yuv420p, convert it to yuv420p.
        sws_scale(swsContext, avFrame->data, avFrame->linesize, 0, avFrame->height, videoDstData, videoDstLineSize);
    }
    // 第一种方式：使用av_image_copy拷贝
    /*
     * copy decoded frame to destination buffer:
     * this is required since rawvideo expects non-aligned data
     * */
    av_image_copy(videoDstData, videoDstLineSize, const_cast<const uint8_t **>(frame->data),
                  frame->linesize, pix_fmt, width, height);

    // write to rawvideo file
    videoOutputFile.write(reinterpret_cast<const char *>(videoDstData[0]), videoDstBufSize);

    // 第二种方式：自己判断写入



    return 0;
}

