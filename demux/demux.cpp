//
// Created by zippozeng on 2023/10/7.
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <utils/constant.hpp>

#include "demuxer.hpp"
#include "adts.hpp"


extern "C" {
#include "libavutil/pixdesc.h"
#include "libavcodec/avcodec.h"
}

demuxer::demuxer() {
    // 1. 创建AVFormatContext
    formatCtx = avformat_alloc_context();
    avBitStreamFilter = av_bsf_get_by_name("h264_mp4toannexb");
}

demuxer::~demuxer() {
    if (formatCtx != nullptr) {
        avformat_free_context(formatCtx);
        formatCtx = nullptr;
    }
    if (avBitStreamFilter != nullptr) {
        avBitStreamFilter = nullptr;
    }
    if (bsfCtx != nullptr) {
        av_bsf_free(&bsfCtx);
        bsfCtx = nullptr;
    }
}

void demuxer::demux(std::string &filename) {
    int audio_index;
    int video_index;
    int ret;
    int total_seconds, hour, minute, second;
    std::string directoryName;
    directoryName.append(PROJECT_PATH);
    directoryName.append("/out");

    if (!avBitStreamFilter) {
        std::cerr << "get AVBitStreamFilter failed" << std::endl;
        return;
    }
    std::filesystem::path path = std::filesystem::current_path();
    std::cout << "path:" << path.string() << std::endl;

    // 检查文件夹是否存在，如果不存在则创建
    if (!std::filesystem::exists(directoryName)) {
        // 如果文件夹不存在，使用 std::filesystem::create_directory() 来创建它
        if (std::filesystem::create_directory(directoryName)) {
            std::cout << "文件夹创建成功:" << directoryName << std::endl;
        } else {
            std::cerr << "文件夹创建失败:" << directoryName << std::endl;
            return;
        }
    }
    std::string audioOutFilePath;
    audioOutFilePath.append(directoryName);
    audioOutFilePath.append("/mp4_extra_aac_audio.aac");
    std::cout << "audio path:" << audioOutFilePath << std::endl;

    std::string videoOutFilePath;
    videoOutFilePath.append(directoryName);
    videoOutFilePath.append("/mp4_extra_h264_video.h264");
    std::cout << "video path:" << videoOutFilePath << std::endl;

    std::ofstream audioOutputFile(audioOutFilePath, std::ios::binary);
    std::ofstream videoOutputFile(videoOutFilePath, std::ios::binary);

    if (!audioOutputFile.is_open()) {
        std::cerr << "无法打开文件:" << audioOutFilePath << std::endl;
        return; // 返回错误码
    }
    if (!videoOutputFile.is_open()) {
        std::cerr << "无法打开文件:" << videoOutFilePath << std::endl;
        return; // 返回错误码
    }
    // 2. 打开文件
    ret = avformat_open_input(&formatCtx, filename.c_str(), nullptr, nullptr);
    if (ret) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cout << "open " << filename << " failed:" << buf << std::endl;
        return;
    }
    // 2.1 探测数据格式
    ret = avformat_find_stream_info(formatCtx, nullptr);
    if (ret < 0) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cout << "find stream info failed:" << buf << std::endl;
        return;
    }
    std::cout << "==== av_dump_format in_filename:" << filename << " ===" << std::endl;
//    av_dump_format(formatCtx, 0, filename.c_str(), 0);
    printf("==== av_dump_format finish =======\n");
    // 2.2 打印信息：name、streams、媒体码率
    std::cout << "media file name:" << filename << std::endl;
    std::cout << "container_format:" << formatCtx->iformat->name << std::endl;
    std::cout << "codec_tag:" << formatCtx->iformat->codec_tag << std::endl;
    std::cout << "nb_streams" << formatCtx->nb_streams << std::endl;
    // 单位为bps，转成kbps
    std::cout << "bit_rate" << formatCtx->bit_rate / 1000 << std::endl;
    // 2.3 计算视频的总时长: hour、minute、second
    // formatCtx->duration的单位为微妙（us），而AV_TIME_BASE为1s的微妙数
    total_seconds = static_cast<int>(formatCtx->duration / AV_TIME_BASE);
    hour = total_seconds / 3600;
    minute = (total_seconds % 3600) / 60;
    second = (total_seconds % 60);
    std::cout << "duration:" << hour << ":" << minute << ":" << second << std::endl;

    // 3. 查找track索引
//    /*
//     * 老版本通过遍历的方式读取媒体文件视频和音频的信息
//     * 新版本的FFmpeg新增加了函数av_find_best_stream，也可以取得同样的效果
//     */
//    for (int i = 0; i < formatCtx->nb_streams; ++i) {
//        AVStream *stream = formatCtx->streams[i];// 音频流、视频流、字幕流
//        //如果是音频流，则打印音频的信息
//        if (AVMEDIA_TYPE_AUDIO == stream->codecpar->codec_type) {
//            printAudio(stream);
//            audio_index = i; // 获取音频的索引
//        } else if (AVMEDIA_TYPE_VIDEO == stream->codecpar->codec_type) {
//            printVideo(stream);
//            video_index = i;
//        }
//    }

    audio_index = av_find_best_stream(formatCtx, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    video_index = av_find_best_stream(formatCtx, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    std::cout << "audio_index:" << audio_index << " video_inex:" << video_index << std::endl;
    // 4. 打印音视频信息
    printAudio(formatCtx->streams[audio_index]);
    printVideo(formatCtx->streams[video_index]);

    // 5. 输出.h264和.aac文件
    packet = av_packet_alloc();
    // 5.1 获取filter
    //4. 初始化过滤器上下文
    ret = av_bsf_alloc(avBitStreamFilter, &bsfCtx);
    avcodec_parameters_copy(bsfCtx->par_in, formatCtx->streams[video_index]->codecpar);
    av_bsf_init(bsfCtx);

    if (ret) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cerr << "av_bsf_alloc error:" << buf << std::endl;
        return;
    }
    std::string container_format(formatCtx->iformat->name);
    while (!av_read_frame(formatCtx, packet)) {
        if (packet->stream_index == audio_index) {
            if (container_format.find("mov,mp4,m4a,3gp,3g2,mj2") != std::string::npos) {
                // 音频，保存到文件
                // 先写adts头，在写data
                ADTSFixedHeader fixedHeader;
                ADTSVariableHeader variableHeader;
                ADTS::set_fixed_header(&fixedHeader,
                                       formatCtx->streams[audio_index]->codecpar->profile,
                                       formatCtx->streams[audio_index]->codecpar->sample_rate,
                                       formatCtx->streams[audio_index]->codecpar->ch_layout.nb_channels);
                ADTS::set_variable_header(&variableHeader, packet->size);
                uint8_t adtsBuf[7] = {0}; // fixedHeader:28bit + variableHeader:28bit = 56bit = 7Byte
                ADTS::convert_adts_header2char(&fixedHeader, &variableHeader, adtsBuf);
//            audioOutputFile.write()
                audioOutputFile.write(reinterpret_cast<char *>(adtsBuf), sizeof(adtsBuf));
                audioOutputFile.write(reinterpret_cast<char *>(packet->data), packet->size);
            } else {
                audioOutputFile.write(reinterpret_cast<char *>(packet->data), packet->size);
            }
        } else if (packet->stream_index == video_index) {
            if (container_format.find("mov,mp4,m4a,3gp,3g2,mj2") != std::string::npos) {
                // 说明是mp4格式类的容器
                av_bsf_send_packet(bsfCtx, packet);
                while (!av_bsf_receive_packet(bsfCtx, packet)) {
                    // 视频，保存到文件
                    videoOutputFile.write(reinterpret_cast<char *>(packet->data), packet->size);
                }
            } else {
                videoOutputFile.write(reinterpret_cast<char *>(packet->data), packet->size);
            }
        }
        av_packet_unref(packet);
    }
}

void demuxer::printAudio(AVStream *stream) {
    std::cout << "----- Audio info start-----" << std::endl;
    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (codec) {
        AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
        if (codecCtx) {
            // 将流的参数复制到解码器上下文
            avcodec_parameters_to_context(codecCtx, stream->codecpar);
            // 获取音频的采样格式
            std::cout << "codec name:" << avcodec_get_name(stream->codecpar->codec_id) << std::endl;
            std::cout << "sample_rate:" << stream->codecpar->sample_rate << std::endl;
            std::cout << "bit depth:" << av_get_bytes_per_sample(codecCtx->sample_fmt) * 8 << std::endl;
            std::cout << "channel:" << stream->codecpar->ch_layout.nb_channels << std::endl;
            std::cout << "format:" << av_get_sample_fmt_name(codecCtx->sample_fmt) << std::endl;
            std::cout << "profile_ObjectType:" << avcodec_profile_name(stream->codecpar->codec_id, codecCtx->profile)
                      << std::endl;
            // 音频总时长，单位为秒。注意如果把单位放大为毫秒或者微妙，音频总时长跟视频总时长不一定相等的
            if (stream->duration != AV_NOPTS_VALUE) {
                double duration = static_cast<double>(stream->duration) * av_q2d(stream->time_base);
                //将音频总时长转换为时分秒的格式打印到控制台上
                std::cout << "audio duration: " << formatTime(duration) << std::endl;
            } else {
                std::cout << "audio duration unknown" << std::endl;
            }
        }
    }
    std::cout << "----- Audio info end-----" << std::endl;
}

void demuxer::printVideo(AVStream *stream) {
    std::cout << "----- Video info start-----" << std::endl;
    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (codec) {
        AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
        if (codecCtx) {
            // 将流的参数复制到解码器上下文
            avcodec_parameters_to_context(codecCtx, stream->codecpar);
            // 获取音频的采样格式
            std::cout << "codec name:" << avcodec_get_name(stream->codecpar->codec_id) << std::endl;
            std::cout << "bit_rate:" << stream->codecpar->bit_rate << std::endl;
            std::cout << "resolution:" << stream->codecpar->width << "x" << stream->codecpar->height << std::endl;
            // avg_frame_rate: 视频帧率,单位为fps，表示每秒出现多少帧
            std::cout << "frame rate:" << av_q2d(stream->avg_frame_rate) << std::endl;
            std::cout << "format:" << av_get_pix_fmt_name(codecCtx->pix_fmt) << std::endl;
            std::cout << "profile_ObjectType:" << avcodec_profile_name(stream->codecpar->codec_id, codecCtx->profile)
                      << std::endl;
            std::cout << "level:" << stream->codecpar->level << std::endl;
            // 视频总时长，单位为秒。注意如果把单位放大为毫秒或者微妙，音频总时长跟视频总时长不一定相等的
            if (stream->duration != AV_NOPTS_VALUE) {
                double duration = static_cast<double>(stream->duration) * av_q2d(stream->time_base);
                //将视频总时长转换为时分秒的格式打印到控制台上
                std::cout << "video duration: " << formatTime(duration) << std::endl;
            } else {
                std::cout << "video duration unknown" << std::endl;
            }
        }
    }
    std::cout << "----- Video info end-----" << std::endl;
}

std::string demuxer::formatTime(double totalSecond) {
    int hours = static_cast<int>(totalSecond) / 3600;
    int minutes = (static_cast<int>(totalSecond) % 3600) / 60;
    int seconds = (static_cast<int>(totalSecond) % 60);
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << hours << ":"
       << std::setw(2) << std::setfill('0') << minutes << ":"
       << std::setw(2) << std::setfill('0') << seconds;
    return ss.str();
}