//
// Created by zippozeng on 2024/2/23.
//

#include "VideoEncoder.hpp"


#include <iostream>
#include <fstream>
#include <utils/constant.hpp>

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

VideoEncoder::~VideoEncoder() {

}

int VideoEncoder::encode(std::string &filename, std::string &out_file_name) {
    int ret;

    // 1. 查找编码器
    enum AVCodecID codecId = AV_CODEC_ID_H264;
    const AVCodec *codec = avcodec_find_encoder(codecId);
    if (!codec) {
        std::cerr << "Codec not found codec" << std::endl;
        exit(1);
    }

    // 2.创建编码器上下文
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "Codec not found AVCodecContext" << std::endl;
        exit(1);
    }
    // 3. 设置编码器参数
    codec_ctx->codec_id = codecId;
    // 3.1 设置分辨率，这里要注意，如果不做scale的话，分辨率要跟原分辨率保持一致
    codec_ctx->width = 576;
    codec_ctx->height = 1024;
    // 3.2 设置time_base
    codec_ctx->time_base = (AVRational) {1, 25};
    codec_ctx->framerate = (AVRational) {25, 1};
    // 设置I帧间隔：如果frame->pict_type设置为AV_PICTURE_TYPE_I, 则忽略gop_size的设置，一直当做I帧进行编码
    codec_ctx->gop_size = 25;
    codec_ctx->max_b_frames = 2; // 如果不想包含B帧则设置为0
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    if (codec_ctx->codec_id == AV_CODEC_ID_H264) {
        // 相关的参数可以参考libx264.c的 AVOption options
        // 编码速度相关
        ret = av_opt_set(codec_ctx->priv_data, "preset", "slow", 0); // 默认为medium
        if (ret != 0) {
            std::cerr << "av_opt_set preset failed" << std::endl;
        }
        // 画质相关，B帧相关
        ret = av_opt_set(codec_ctx->priv_data, "profile", "main", 0); // 默认是high
        if (ret != 0) {
            std::cerr << "av_opt_set profile failed" << std::endl;
        }
        // 画质相关
        av_opt_set(codec_ctx->priv_data, "tune", "zerolatency", 0);
        if (ret != 0) {
            std::cerr << "av_opt_set tune failed" << std::endl;
        }
    }
    // 设置码率: 3M
    codec_ctx->bit_rate = 3 * 1000 * 1000;
    //    codec_ctx->rc_max_rate = 3000000;
//    codec_ctx->rc_min_rate = 3000000;
//    codec_ctx->rc_buffer_size = 2000000;
//    codec_ctx->thread_count = 4;  // 开了多线程后也会导致帧输出延迟, 需要缓存thread_count帧后再编程。
//    codec_ctx->thread_type = FF_THREAD_FRAME; // 并 设置为FF_THREAD_FRAME
    /* 对于H264 AV_CODEC_FLAG_GLOBAL_HEADER  设置则只包含I帧，此时sps pps需要从codec_ctx->extradata读取
     *  不设置则每个I帧都带 sps pps sei
     */
//    codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; // 存本地文件时不要去设置

    std::cout << "------Video encode config print start------" << std::endl;
    std::cout << "width: " << codec_ctx->width << std::endl;
    std::cout << "height: " << codec_ctx->height << std::endl;
    std::cout << "bitrate: " << codec_ctx->bit_rate << std::endl;
    std::cout << "framerate: " << codec_ctx->framerate.num << std::endl;
    std::cout << "gop_size: " << codec_ctx->gop_size << std::endl;
    std::cout << "------Video encode config print end------" << std::endl;

    // 关联解码器上下文
    ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
        std::cerr << "avcodec_open2 failed" << std::endl;
        exit(1);
    }
    // 读取文件
    std::ifstream inputFile(filename, std::ios::binary);
    std::cout << "inputFile:" << filename << std::endl;
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open the file:" << filename << std::endl;
        return 1;
    }

    std::string directoryName;
    directoryName.append(PROJECT_PATH);
    directoryName.append("/out");
    std::filesystem::path path = std::filesystem::current_path();
    std::cout << "path:" << path.string() << std::endl;

    // 检查文件夹是否存在，如果不存在则创建
    if (!std::filesystem::exists(directoryName)) {
        // 如果文件夹不存在，使用 std::filesystem::create_directory() 来创建它
        if (std::filesystem::create_directory(directoryName)) {
            std::cout << "文件夹创建成功:" << directoryName << std::endl;
        } else {
            std::cerr << "文件夹创建失败:" << directoryName << std::endl;
            return 1;
        }
    }

    std::string h264OutFilePath;
    h264OutFilePath.append(directoryName);
    h264OutFilePath.append(out_file_name);
    std::cout << "video h264 path:" << h264OutFilePath << std::endl;

    std::ofstream videoOutputFile(h264OutFilePath, std::ios::binary);
    if (!videoOutputFile.is_open()) {
        std::cerr << "无法打开文件:" << h264OutFilePath << std::endl;
        return 1; // 返回错误码
    }
    // 分配pkt和frame
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "Could not allocate video packet" << std::endl;
        return 1;
    }
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Could not allocate video frame" << std::endl;
        return 1;
    }
    // 为frame分配buffer
    frame->format = codec_ctx->pix_fmt;
    frame->width = codec_ctx->width;
    frame->height = codec_ctx->height;
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        std::cerr << "Could not allocate the video frame data" << std::endl;
        exit(1);
    }
    std::cout << "start encode" << std::endl;
    // 计算出每一帧的数据 单个采样点的字节 * 通道数目 * 每帧采样点数量
    int frame_bytes = av_image_get_buffer_size(static_cast<AVPixelFormat>(frame->format), frame->width, frame->height,
                                               1);
    std::cout << "frame_bytes:" << frame_bytes << std::endl;
    auto *yuv_buf = new uint8_t[frame_bytes];
    int64_t startTime = getTime();
    int64_t endTime = startTime;
    int64_t allStartTime = getTime();
    int64_t allEndTime = allStartTime;
    int64_t pts = 0;
    while (!inputFile.eof()) {
        memset(yuv_buf, 0, frame_bytes);
        inputFile.read(reinterpret_cast<char *>(yuv_buf), frame_bytes);
        size_t bytesRead = inputFile.gcount(); // 获取实际读取的字节数
        if (bytesRead <= 0) {
            break;
        }
        /* 确保该frame可写, 如果编码器内部保持了内存参考计数，则需要重新拷贝一个备份
         目的是新写入的数据和编码器保存的数据不能产生冲突
        */
        int frame_is_writable = 1;
        if (av_frame_is_writable(frame) == 0) { // 这里只是用来测试
            std::cout << "the frame can't write, buf:" << frame->buf[0] << std::endl;
            if (frame->buf && frame->buf[0])        // 打印referenc-counted，必须保证传入的是有效指针
                std::cout << "ref_count1(frame) = %d" << av_buffer_get_ref_count(frame->buf[0]) << std::endl;
            frame_is_writable = 0;
        }
        ret = av_frame_make_writable(frame);
        if (frame_is_writable == 0) {  // 这里只是用来测试
//            std::cout << "av_frame_make_writable, buf:", reinterpret_cast<intptr_t>(frame->buf[0]) << std::endl;
            if (frame->buf) {
                if (frame->buf[0] != nullptr) {   // 打印referenc-counted，必须保证传入的是有效指针
                    std::cout << "ref_count2(frame) = " << av_buffer_get_ref_count(frame->buf[0]) << std::endl;
                }
            }
        }
        if (ret != 0) {
            std::cerr << "av_frame_make_writable failed, ret = " << ret << std::endl;
            break;
        }
        int needSize = av_image_fill_arrays(frame->data, frame->linesize, yuv_buf,
                                            static_cast<AVPixelFormat>(frame->format), frame->width,
                                            frame->height, 1);
        if (needSize != frame_bytes) {
            std::cerr << "av_image_fill_arrays failed, "
                         "need_size:" << needSize << ", frame_bytes:" << frame_bytes << std::endl;
            break;
        }
        pts += 40;
        // 设置pts
        frame->pts = pts; // 使用采样率作为pts的单位，具体换算成秒 pts*1/采样率
        startTime = getTime();
        ret = encodeInternal(codec_ctx, frame, packet, videoOutputFile);
        endTime = getTime();
        std::cout << "encode cost time:" << endTime - startTime << std::endl;
        if (ret < 0) {
            std::cerr << "encode failed" << std::endl;
            break;
        }
    }
    // 冲刷编码器
    encodeInternal(codec_ctx, nullptr, packet, videoOutputFile);
    allEndTime = getTime();
    std::cout << "all encode cost time:" << allEndTime - allStartTime << std::endl;
    inputFile.close();
    videoOutputFile.close();
    // 释放资源
    delete[] yuv_buf;
    std::cout << "main finish, please enter Enter and exit" << std::endl;
    return 0;
}

int VideoEncoder::encodeInternal(AVCodecContext *codec_ctx, AVFrame *frame, AVPacket *pkt, std::ofstream &outfile) {
    int ret;
    ret = avcodec_send_frame(codec_ctx, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return 0;
    } else if (ret < 0) {
        std::cerr << "Error encoding video frame" << std::endl;
        return -1;
    }
    /* read all the available output packets (in general there may be any number of them */
    // 编码和解码都是一样的，都是send 1次，然后receive多次, 直到AVERROR(EAGAIN)或者AVERROR_EOF
    while (true) {
        ret = avcodec_receive_packet(codec_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        } else if (ret < 0) {
            fprintf(stderr, "Error encoding audio frame\n");
            return -1;
        }
        if (pkt->flags & AV_PKT_FLAG_KEY) {
            std::cout << "Write packet flags:" << pkt->flags
                      << " pts:" << pkt->pts
                      << " dts:" << pkt->dts
                      << " (size:" << pkt->size << ")" << std::endl;
        }
        if (!pkt->flags) {
            std::cout << "Write packet flags:" << pkt->flags
                      << " pts:" << pkt->pts
                      << " dts:" << pkt->dts
                      << " (size:" << pkt->size << ")" << std::endl;
        }
        outfile.write(reinterpret_cast<const char *>(pkt->data), pkt->size);
    }
}

int64_t VideoEncoder::getTime() {
    return av_gettime_relative() / 1000;  // 换算成毫秒
}
