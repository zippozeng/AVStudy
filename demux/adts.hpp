//
// Created by zippozeng on 2023/10/7.
//

#ifndef AVSTUDY_ADTS_HPP
#define AVSTUDY_ADTS_HPP

#include <cstdint>
#include <bitset>

typedef struct ADTSFixedHeader {
public:
    uint16_t syncword: 12;
    uint8_t id: 1;
    uint8_t layer: 2;
    uint8_t protection_absent: 1;
    uint8_t profile_ObjectType: 2;
    uint8_t sampling_frequency_index: 4;
    uint8_t private_bit: 1;
    uint8_t channel_configuration: 3;
    uint8_t original_copy: 1;
    uint8_t home: 1;
} ADTSFixedHeader; // length: 28 bits

typedef struct ADTSVariableHeader {
    uint8_t copyright_identification_bit: 1;
    uint8_t copyright_identification_start: 1;
    uint16_t aac_frame_length: 13;
    uint16_t adts_buffer_fullness: 11;
    uint8_t number_of_raw_data_blocks_in_frame: 2;
} ADTSVariableHeader; // length : 28 bits

const int sampling_frequencies[] = {
        96000,  // 0x0
        88200,  // 0x1
        64000,  // 0x2
        48000,  // 0x3
        44100,  // 0x4
        32000,  // 0x5
        24000,  // 0x6
        22050,  // 0x7
        16000,  // 0x8
        12000,  // 0x9
        11025,  // 0xa
        8000   // 0xb
        // 0xc d e f是保留的
};

class ADTS {
public:
    // 设置adts固定的一部分
    static void set_fixed_header(ADTSFixedHeader *header, const int profile, const int sampleRate,
                                 const int channels);

    // 设置adts可变的一部分
    static void set_variable_header(ADTSVariableHeader *header, int aacRawDataLength);

    // 解析buff中的adts固定部分头信息
    static void get_fixed_header(const unsigned char *buff, ADTSFixedHeader *header);

    // 解析buff中的adts可变部分头信息
    static void get_variable_header(const unsigned char *buff, ADTSVariableHeader *header);


    // 将adts头信息转化为一个7字节的buff
    static void convert_adts_header2char(const ADTSFixedHeader *fixed_header,
                                         const ADTSVariableHeader *variable_header,
                                         uint8_t *buffer);

    void adts_header(char *p_adts_header, int data_length);

    void extract_aac();

private:
    // 将adts头信息转化为一个64位的整数, PS: 前面8位空着
    static void convert_adts_header2int64(const ADTSFixedHeader *fixed_header,
                                          const ADTSVariableHeader *variable_header,
                                          uint64_t *header);
};


#endif //AVSTUDY_ADTS_HPP
