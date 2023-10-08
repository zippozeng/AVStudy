//
// Created by zippozeng on 2023/10/7.
//

#include "adts.hpp"
#include <iostream>

void ADTS::set_fixed_header(ADTSFixedHeader *header, const int profile, const int sampleRate,
                            const int channels) {
    //    syncword ：同步头 总是0xFFF, all bits must be 1，代表着⼀个ADTS帧的开始
    //    ID：MPEG标识符，0标识MPEG-4，1标识MPEG-2
    //    Layer：always: 00
    //    protection_absent：表示是否误码校验。Warning, set to 1 if there is no CRC and 0 if there is CRC
    //    profile_ObjectType：表示使⽤哪个级别的AAC
    // 遍历，查找sampling_frequencies表
    int sampling_frequency_index = -1;
    int frequencies_size = sizeof(sampling_frequencies) / sizeof(sampling_frequencies[0]);
    for (int i = 0; i < frequencies_size; ++i) {
        if (sampling_frequencies[i] == sampleRate) {
            sampling_frequency_index = i;
            break;
        }
    }
    if (sampling_frequency_index == -1) {
        std::cout << "un support sample rate:" << std::endl;
        return;
    }

    header->syncword = 0xFFF;
    header->id = 0; // TODO:MPEG Version: 0 for MPEG-4, 1 for MPEG-2
    header->layer = 0;// always 00
    header->protection_absent = 1;// TODO:表示是否误码校验
    header->profile_ObjectType = profile;// 表示使用哪个级别的AAC, 有些芯片只支持AAC LC.
    header->sampling_frequency_index = sampling_frequency_index;// 表示使用的采样率的下标
    header->private_bit = 0;
    header->channel_configuration = channels;// 表示声道数
    header->original_copy = 0;
    header->home = 0;
}

void ADTS::set_variable_header(ADTSVariableHeader *header, int aacRawDataLength) {
    header->copyright_identification_bit = 0;
    header->copyright_identification_start = 0;
    // aac_frame_length = (protection_absent == 1 ? 7 : 9) + size(AACFrame)
    header->aac_frame_length = aacRawDataLength + 7;
    header->adts_buffer_fullness = 0x7f;
    header->number_of_raw_data_blocks_in_frame = 2;
}

void ADTS::get_fixed_header(const unsigned char *buff, ADTSFixedHeader *header) {

}

void ADTS::get_variable_header(const unsigned char *buff, ADTSVariableHeader *header) {

}

void ADTS::convert_adts_header2int64(const ADTSFixedHeader *fixed_header, const ADTSVariableHeader *variable_header,
                                     uint64_t *header) {
    uint64_t retValue;
    retValue = fixed_header->syncword;
    // id
    retValue <<= 1;
    retValue |= fixed_header->id;
    // layer
    retValue <<= 2;
    retValue |= fixed_header->layer;
    // protection_absent
    retValue <<= 1;
    retValue |= fixed_header->protection_absent;
    // profile_ObjectType
    retValue <<= 2;
    retValue |= fixed_header->profile_ObjectType;
    // sampling_frequency_index
    retValue <<= 4;
    retValue |= fixed_header->sampling_frequency_index;
    // private_bit
    retValue <<= 1;
    retValue |= fixed_header->private_bit;
    // channel_configuration
    retValue <<= 3;
    retValue |= fixed_header->channel_configuration;
    // original_copy
    retValue <<= 1;
    retValue |= fixed_header->original_copy;
    // home
    retValue <<= 1;
    retValue |= fixed_header->home;

    // variable_header
    retValue <<= 1;
    retValue |= (variable_header->copyright_identification_bit) & 0x01;
    retValue <<= 1;
    retValue |= (variable_header->copyright_identification_start) & 0x01;
    retValue <<= 13;
    retValue |= variable_header->aac_frame_length;
    retValue <<= 11;
    retValue |= variable_header->adts_buffer_fullness;
    retValue <<= 2;
    retValue |= variable_header->number_of_raw_data_blocks_in_frame;

    *header = retValue;
}

void ADTS::convert_adts_header2char(const ADTSFixedHeader *fixed_header, const ADTSVariableHeader *variable_header,
                                    uint8_t *buffer) {
    //
    uint64_t value = 0;
    convert_adts_header2int64(fixed_header, variable_header, &value);

    buffer[0] = (value >> 48) & 0xff;
    buffer[1] = (value >> 40) & 0xff;
    buffer[2] = (value >> 32) & 0xff;
    buffer[3] = (value >> 24) & 0xff;
    buffer[4] = (value >> 16) & 0xff;
    buffer[5] = (value >> 8) & 0xff;
    buffer[6] = (value) & 0xff;
}

void ADTS::adts_header(char *p_adts_header, int data_length) {

}

void ADTS::extract_aac() {

}
