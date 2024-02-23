//
// Created by zippozeng on 2024/2/22.
//

#include "AudioEncoder.hpp"


#include <fstream>
#include <iostream>
#include <utils/constant.hpp>

AudioEncoder::~AudioEncoder() {
    if (frame != nullptr) {
        av_frame_free(&frame);
        frame = nullptr;
    }
    if (packet != nullptr) {
        av_packet_free(&packet);
        packet = nullptr;
    }
    if (codec_ctx != nullptr) {
        avcodec_free_context(&codec_ctx);
        codec_ctx = nullptr;
    }
}

int AudioEncoder::encode(std::string &filename, std::string &out_file_name) {
    int ret;

    // 1. 查找编码器
    enum AVCodecID avCodecId = AV_CODEC_ID_AAC;

    codec = avcodec_find_encoder(avCodecId);
    if (!codec) {
        std::cerr << "Codec not found:" << avcodec_get_name(avCodecId) << std::endl;
        return 3; // 返回错误码
    }

    // 2.创建编码器上下文
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "Could not allocate audio codec context" << std::endl;
        exit(1);
    }

    // 3.设置编码器参数
    codec_ctx->codec_id = avCodecId; // 设置采样格式为浮点数
    codec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    codec_ctx->bit_rate = 128 * 1024; // 设置比特率
    codec_ctx->sample_rate = 48000; // 设置采样率
    // 老版本
    codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
    codec_ctx->channels = av_get_channel_layout_nb_channels(codec_ctx->channel_layout);
    // 新版本
//    codec_ctx->ch_layout = (AVChannelLayout) AV_CHANNEL_LAYOUT_STEREO;
//    codec_ctx->profile = FF_PROFILE_AAC_LOW;

    // 根据编码器名称来设置sample_fmt
    if (strcmp(codec->name, "aac") == 0) {
        codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    } else if (strcmp(codec->name, "libfdk_aac") == 0) {
        codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
    } else {
        codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    }
    codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP; // 设置采样格式为浮点数
    // 检测支持采样格式支持情况
    if (!checkSampleFmt(codec, codec_ctx->sample_fmt)) {
        std::cerr << "Encoder does not support sample format "
                  << av_get_sample_fmt_name(codec_ctx->sample_fmt) << std::endl;
        exit(1);
    }

    /* select other audio parameters supported by the encode */
//    codec_ctx->sample_rate    = select_sample_rate(codec);
    if (!checkSampleRate(codec, codec_ctx->sample_rate)) {
        std::cerr << "Encoder does not support sample rate " << codec_ctx->sample_rate << std::endl;
        exit(1);
    }

//    if (!checkChannelLayout(codec, codec_ctx->ch_layout)) {
//        std::cerr << "Encoder does not support channel layout " << codec_ctx->channel_layout << std::endl;
//        exit(1);
//    }

    std::cout << "------Audio encode config print start------" << std::endl;
    std::cout << "bit_rate: " << codec_ctx->bit_rate / 1024 << "kbps" << std::endl;
    std::cout << "sample_rate:" << codec_ctx->sample_rate << std::endl;
    std::cout << "sample_fmt:" << av_get_sample_fmt_name(codec_ctx->sample_fmt) << std::endl;
    std::cout << "channels:" << codec_ctx->channels << std::endl;
    std::cout << "1 frame_size:" << codec_ctx->frame_size << std::endl;
    std::cout << "------Audio encode config print end------" << std::endl;
    codec_ctx->flags = AV_CODEC_FLAG_GLOBAL_HEADER;  //ffmpeg默认的aac是不带adts，而fdk_aac默认带adts，这里我们强制不带

    ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret) {
        std::cerr << "Failed to open audio encode!" << std::endl;
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

    std::string aacOutFilePath;
    aacOutFilePath.append(directoryName);
    aacOutFilePath.append(out_file_name);
    std::cout << "audio path:" << aacOutFilePath << std::endl;

    std::ofstream audioOutputFile(aacOutFilePath, std::ios::binary);
    if (!audioOutputFile.is_open()) {
        std::cerr << "无法打开文件:" << aacOutFilePath << std::endl;
        return 2; // 返回错误码
    }

    packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "Failed to allocate AVPacket!" << std::endl;
        exit(1);
    }
    frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Failed to allocate AVFrame!" << std::endl;
        exit(1);
    }

    /* 每次送多少数据给编码器由：
    *  (1)frame_size(每帧单个通道的采样点数);
    *  (2)sample_fmt(采样点格式);
    *  (3)channel_layout(通道布局情况);
    * 3要素决定
    */
    frame->nb_samples = codec_ctx->frame_size;
    frame->format = codec_ctx->sample_fmt;
    frame->channel_layout = codec_ctx->channel_layout;
    frame->channels = codec_ctx->channels;

    std::cout << "------Audio frame config print start------" << std::endl;
    std::cout << "frame nb_samples:" << frame->nb_samples << std::endl;
    std::cout << "frame sample_fmt:" << av_get_sample_fmt_name((AVSampleFormat) frame->format) << std::endl;
    std::cout << "frame channel_layout:" << frame->channel_layout << std::endl;
    std::cout << "frame nb_channels:" << frame->channels << std::endl;
    std::cout << "------Audio frame config print end------" << std::endl;
    /* 为frame分配buffer */
    ret = av_frame_get_buffer(frame, 0);
    if (ret) {
        std::cout << "Could not allocate audio data buffers ret:" << ret << std::endl;
        exit(1);
    }
    // 计算出每一帧的数据 单个采样点的字节 * 通道数目 * 每帧采样点数量
    int frame_bytes = av_get_bytes_per_sample(codec_ctx->sample_fmt) * frame->channels * frame->nb_samples;
    std::cout << "frame_bytes:" << frame_bytes << std::endl;
    auto *pcm_buf = new uint8_t[frame_bytes];
    auto *pcm_temp_buf = new uint8_t[frame_bytes];
    int64_t pts = 0;
    // 读取pcm数据
    std::cout << "start encode" << std::endl;
    while (!inputFile.eof()) {
        memset(pcm_buf, 0, frame_bytes);
        inputFile.read(reinterpret_cast<char *>(pcm_buf), frame_bytes); // 从文件中读取最多1024个字节
        size_t bytesRead = inputFile.gcount(); // 获取实际读取的字节数

        if (bytesRead <= 0) {
            break;
        }

        // 确保保该frame可写, 如果编码器内部保持了内存参考计数，则需要重新拷贝一个备份
        // 目的是新写入的数据和编码器保存的数据不能产生冲突
        ret = av_frame_make_writable(frame);
        if (ret) {
            std::cout << "av_frame_make_writable failed, ret = " << ret << std::endl;
            exit(1);
        }
        if (AV_SAMPLE_FMT_S16 == frame->format) {
            // 将读取到的PCM数据填充到frame去，但要注意格式的匹配, 是planar还是packed都要区分清楚
            ret = av_samples_fill_arrays(frame->data, frame->linesize,
                                         pcm_buf, frame->channels,
                                         frame->nb_samples, static_cast<AVSampleFormat>(frame->format), 0);
        } else {
            // 将读取到的PCM数据填充到frame去，但要注意格式的匹配, 是planar还是packed都要区分清楚
            // 将本地的f32le packed模式的数据转为float palanar
            memset(pcm_temp_buf, 0, frame_bytes);
            f32le_convert_to_fltp((float *) pcm_buf, (float *) pcm_temp_buf, frame->nb_samples);
            av_samples_fill_arrays(frame->data, frame->linesize,
                                   pcm_temp_buf, frame->channels,
                                   frame->nb_samples, static_cast<AVSampleFormat>(frame->format), 0);
        }
        // 设置pts
        pts += frame->nb_samples;
        frame->pts = pts;       // 使用采样率作为pts的单位，具体换算成秒 pts*1/采样率
        ret = encodeInternal(codec_ctx, frame, packet, audioOutputFile);
        if (ret < 0) {
            std::cout << "encode failed" << std::endl;
            break;
        }
//        if (bytesRead < 1024) {
//            // 处理最后一次读取不足1024个字节的情况
//            // 在这里 bytesRead 就是最后一次读取的字节数
//        }
    }
    /* 冲刷编码器 */
    encodeInternal(codec_ctx, nullptr, packet, audioOutputFile);

    // 释放内存
    delete[] pcm_buf;
    delete[] pcm_temp_buf;
    return 0;
}

int AudioEncoder::checkSampleFmt(const AVCodec *codec, enum AVSampleFormat sample_fmt) {
    const enum AVSampleFormat *p = codec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

int AudioEncoder::checkSampleRate(const AVCodec *codec, const int sample_rate) {
    const int *p = codec->supported_samplerates;
    if (!p) {
        return 0;
    }
    while (*p != 0) {
        std::cout << codec->name << " support " << *p << "hz" << std::endl;
        if (*p == sample_rate)
            return 1;
        p++;
    }
    return 0;
}

int AudioEncoder::checkChannelLayout(const AVCodec *codec, AVChannelLayout &channel_layout) {
    // 不是每个codec都给出支持的channel_layout
//    const AVChannelLayout *p = codec->ch_layouts;
//    if (!p) {
//        std::cout << "the codec " << codec->name << " no set channel_layouts" << std::endl;
//        return 1;
//    }
//    while (p != nullptr) { // 0作为退出条件，比如libfdk-aacenc.c的aac_channel_layout
//        std::cout << codec->name << " support channel_layout " << *p << std::endl;
//        if (*p == const_cast<AVChannelLayout>(channel_layout))
//            return 1;
//        p++;
//    }
    return 1;
}

int AudioEncoder::select_sample_rate(const AVCodec *codec) {
    const int *p;
    int best_samplerate = 0;

    if (!codec->supported_samplerates)
        return 44100;

    p = codec->supported_samplerates;
    while (*p) {
        if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
            best_samplerate = *p;
        p++;
    }
    return best_samplerate;
}

void AudioEncoder::f32le_convert_to_fltp(const float *f32le, float *fltp, int nb_samples) {
    float *fltp_l = fltp;   // 左通道
    float *fltp_r = fltp + nb_samples;   // 右通道
    for (int i = 0; i < nb_samples; i++) {
        fltp_l[i] = f32le[i * 2];     // 0 1   - 2 3
        fltp_r[i] = f32le[i * 2 + 1];   // 可以尝试注释左声道或者右声道听听声音
    }
}

int AudioEncoder::encodeInternal(AVCodecContext *codec_ctx, AVFrame *frame, AVPacket *pkt, std::ofstream &outfile) {
    int ret;
    ret = avcodec_send_frame(codec_ctx, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return 0;
    } else if (ret < 0) {
        std::cerr << "Error encoding audio frame" << std::endl;
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

        size_t len = 0;
        std::cout << "ctx->flags:0x" << codec_ctx->flags
                  << " & AV_CODEC_FLAG_GLOBAL_HEADER:0x" << (codec_ctx->flags & AV_CODEC_FLAG_GLOBAL_HEADER)
                  << ", name:" << codec_ctx->codec->name << std::endl;
        if ((codec_ctx->flags & AV_CODEC_FLAG_GLOBAL_HEADER)) {
            // 需要额外的adts header写入
            uint8_t aac_header[7];
            getAdtsHeader(codec_ctx, aac_header, pkt->size);
            std::streampos startpos = outfile.tellp(); // 记录初始位置
            outfile.write(reinterpret_cast<char *>(aac_header), 7);
            std::streampos endpos = outfile.tellp(); // 获取写入后的位置
            len = endpos - startpos; // 计算写入的字节数
            if (len != 7) {
                std::cout << "ostream aac_header failed" << std::endl;
                return -1;
            }
        }
        std::streampos startpos = outfile.tellp(); // 记录初始位置
        outfile.write(reinterpret_cast<char *>(pkt->data), pkt->size);
        std::streampos endpos = outfile.tellp(); // 获取写入后的位置
        len = endpos - startpos; // 计算写入的字节数
        if (len != pkt->size) {
            fprintf(stderr, "write aac data failed\n");
            return -1;
        }
        /* 是否需要释放数据? avcodec_receive_packet第一个调用的就是 av_packet_unref
        * 所以我们不用手动去释放，这里有个问题，不能将pkt直接插入到队列，因为编码器会释放数据
        * 可以新分配一个pkt, 然后使用av_packet_move_ref转移pkt对应的buffer
        */
        // av_packet_unref(pkt);
    }
}

void AudioEncoder::getAdtsHeader(AVCodecContext *ctx, uint8_t *adts_header, int aac_length) {
    uint8_t freq_idx = 0;    //0: 96000 Hz  3: 48000 Hz 4: 44100 Hz
    switch (ctx->sample_rate) {
        case 96000:
            freq_idx = 0;
            break;
        case 88200:
            freq_idx = 1;
            break;
        case 64000:
            freq_idx = 2;
            break;
        case 48000:
            freq_idx = 3;
            break;
        case 44100:
            freq_idx = 4;
            break;
        case 32000:
            freq_idx = 5;
            break;
        case 24000:
            freq_idx = 6;
            break;
        case 22050:
            freq_idx = 7;
            break;
        case 16000:
            freq_idx = 8;
            break;
        case 12000:
            freq_idx = 9;
            break;
        case 11025:
            freq_idx = 10;
            break;
        case 8000:
            freq_idx = 11;
            break;
        case 7350:
            freq_idx = 12;
            break;
        default:
            freq_idx = 4;
            break;
    }
    uint8_t chanCfg = ctx->channels;
    uint32_t frame_length = aac_length + 7;
    adts_header[0] = 0xFF;
    adts_header[1] = 0xF1;
    adts_header[2] = ((ctx->profile) << 6) + (freq_idx << 2) + (chanCfg >> 2);
    adts_header[3] = (((chanCfg & 3) << 6) + (frame_length >> 11));
    adts_header[4] = ((frame_length & 0x7FF) >> 3);
    adts_header[5] = (((frame_length & 7) << 5) + 0x1F);
    adts_header[6] = 0xFC;
}