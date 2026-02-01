#include "frame_analyzer.h"
#include <iostream>

FrameAnalyzer::FrameAnalyzer() {
}

FrameAnalyzer::~FrameAnalyzer() {
}

uint8_t FrameAnalyzer::readBit(const uint8_t* data, size_t size, size_t bitPos) {
    size_t bytePos = bitPos / 8;
    size_t bitInByte = bitPos % 8;
    
    if (bytePos >= size) {
        return 0;
    }
    
    return (data[bytePos] >> (7 - bitInByte)) & 0x01;
}

uint32_t FrameAnalyzer::readBits(const uint8_t* data, size_t size, size_t& bitPos, uint8_t bitCount) {
    uint32_t result = 0;
    for (uint8_t i = 0; i < bitCount; i++) {
        result = (result << 1) | readBit(data, size, bitPos);
        bitPos++;
    }
    return result;
}

uint32_t FrameAnalyzer::parseExpGolomb(const uint8_t* data, size_t size, size_t& bitPos) {
    // 计算前导零的个数
    uint8_t leadingZeros = 0;
    while (leadingZeros < 32 && bitPos < size * 8 && readBit(data, size, bitPos) == 0) {
        leadingZeros++;
        bitPos++;
    }
    
    // 读取后续的 bits
    uint32_t codeNum = 0;
    if (leadingZeros > 0) {
        codeNum = readBits(data, size, bitPos, leadingZeros);
        codeNum += (1 << leadingZeros) - 1;
    }
    
    return codeNum;
}

bool FrameAnalyzer::analyzeFrame(const NALUnit& nalUnit, FrameInfo& frameInfo) {
    // 检查是否为切片类型的 NAL 单元
    if (nalUnit.nal_unit_type < NAL_UNIT_SLICE_TRAIL_N || nalUnit.nal_unit_type > NAL_UNIT_SLICE_GDR) {
        return false;
    }
    
    const uint8_t* data = nalUnit.payload.data();
    size_t size = nalUnit.payload.size();
    size_t bitPos = 0;
    
    // 解析帧信息
    // 1. first_slice_segment_in_pic_flag
    uint8_t firstSliceSegmentInPicFlag = readBit(data, size, bitPos);
    bitPos++;
    
    // 2. slice_type
    frameInfo.slice_type = parseExpGolomb(data, size, bitPos);
    
    // 3. pic_parameter_set_id
    frameInfo.pic_parameter_set_id = parseExpGolomb(data, size, bitPos);
    
    // 4. frame_num
    // 假设 log2_max_frame_num_minus4 = 4 (默认值)
    uint8_t log2MaxFrameNum = 4 + 4;
    frameInfo.frame_num = readBits(data, size, bitPos, log2MaxFrameNum);
    
    // 5. 其他字段
    if (firstSliceSegmentInPicFlag) {
        // 6. pic_order_cnt_lsb
        // 假设 log2_max_pic_order_cnt_lsb_minus4 = 4 (默认值)
        uint8_t log2MaxPicOrderCntLsb = 4 + 4;
        frameInfo.pic_order_cnt_lsb = readBits(data, size, bitPos, log2MaxPicOrderCntLsb);
        
        // 7. delta_poc_bottom
        frameInfo.delta_poc_bottom = parseExpGolomb(data, size, bitPos);
        if (frameInfo.delta_poc_bottom % 2 == 0) {
            frameInfo.delta_poc_bottom = -(frameInfo.delta_poc_bottom / 2);
        } else {
            frameInfo.delta_poc_bottom = (frameInfo.delta_poc_bottom + 1) / 2;
        }
        
        // 8. no_output_of_prior_pics_flag
        frameInfo.no_output_of_prior_pics_flag = readBit(data, size, bitPos);
        bitPos++;
        
        // 9. long_term_reference_flag
        frameInfo.long_term_reference_flag = readBit(data, size, bitPos);
        bitPos++;
    }
    
    // 10. adaptive_ref_pic_marking_mode_flag
    frameInfo.adaptive_ref_pic_marking_mode_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 11. slice_header_extension_present_flag
    frameInfo.slice_header_extension_present_flag = readBit(data, size, bitPos);
    bitPos++;
    
    return true;
}

void FrameAnalyzer::printFrameInfo(const FrameInfo& frameInfo) {
    std::cout << "Frame Information:" << std::endl;
    std::cout << "  Slice Type: " << (int)frameInfo.slice_type << std::endl;
    std::cout << "  Pic Parameter Set ID: " << (int)frameInfo.pic_parameter_set_id << std::endl;
    std::cout << "  Frame Num: " << frameInfo.frame_num << std::endl;
    std::cout << "  Pic Order Cnt LSB: " << frameInfo.pic_order_cnt_lsb << std::endl;
    std::cout << "  Delta POC Bottom: " << frameInfo.delta_poc_bottom << std::endl;
    std::cout << "  No Output of Prior Pics Flag: " << (int)frameInfo.no_output_of_prior_pics_flag << std::endl;
    std::cout << "  Long Term Reference Flag: " << (int)frameInfo.long_term_reference_flag << std::endl;
    std::cout << "  Adaptive Ref Pic Marking Mode Flag: " << (int)frameInfo.adaptive_ref_pic_marking_mode_flag << std::endl;
    std::cout << "  Slice Header Extension Present Flag: " << (int)frameInfo.slice_header_extension_present_flag << std::endl;
    std::cout << std::endl;
}
