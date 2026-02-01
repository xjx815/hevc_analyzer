#include "macroblock_analyzer.h"
#include <iostream>

MacroblockAnalyzer::MacroblockAnalyzer() {
}

MacroblockAnalyzer::~MacroblockAnalyzer() {
}

uint8_t MacroblockAnalyzer::readBit(const uint8_t* data, size_t size, size_t bitPos) {
    size_t bytePos = bitPos / 8;
    size_t bitInByte = bitPos % 8;
    
    if (bytePos >= size) {
        return 0;
    }
    
    return (data[bytePos] >> (7 - bitInByte)) & 0x01;
}

uint32_t MacroblockAnalyzer::readBits(const uint8_t* data, size_t size, size_t& bitPos, uint8_t bitCount) {
    uint32_t result = 0;
    for (uint8_t i = 0; i < bitCount; i++) {
        result = (result << 1) | readBit(data, size, bitPos);
        bitPos++;
    }
    return result;
}

uint32_t MacroblockAnalyzer::parseExpGolomb(const uint8_t* data, size_t size, size_t& bitPos) {
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

bool MacroblockAnalyzer::analyzeMacroblock(const NALUnit& nalUnit, const FrameInfo& frameInfo, std::vector<MacroblockInfo>& mbInfos) {
    // 检查是否为切片类型的 NAL 单元
    if (nalUnit.nal_unit_type < NAL_UNIT_SLICE_TRAIL_N || nalUnit.nal_unit_type > NAL_UNIT_SLICE_GDR) {
        return false;
    }
    
    const uint8_t* data = nalUnit.payload.data();
    size_t size = nalUnit.payload.size();
    size_t bitPos = 0;
    
    // 跳过切片头部信息
    // 1. first_slice_segment_in_pic_flag
    uint8_t firstSliceSegmentInPicFlag = readBit(data, size, bitPos);
    bitPos++;
    
    // 2. slice_type
    parseExpGolomb(data, size, bitPos);
    
    // 3. pic_parameter_set_id
    parseExpGolomb(data, size, bitPos);
    
    // 4. frame_num
    uint8_t log2MaxFrameNum = 4 + 4;
    readBits(data, size, bitPos, log2MaxFrameNum);
    
    // 5. 其他字段
    if (firstSliceSegmentInPicFlag) {
        // 6. pic_order_cnt_lsb
        uint8_t log2MaxPicOrderCntLsb = 4 + 4;
        readBits(data, size, bitPos, log2MaxPicOrderCntLsb);
        
        // 7. delta_poc_bottom
        parseExpGolomb(data, size, bitPos);
        
        // 8. no_output_of_prior_pics_flag
        readBit(data, size, bitPos);
        bitPos++;
        
        // 9. long_term_reference_flag
        readBit(data, size, bitPos);
        bitPos++;
    }
    
    // 10. adaptive_ref_pic_marking_mode_flag
    readBit(data, size, bitPos);
    bitPos++;
    
    // 11. slice_header_extension_present_flag
    readBit(data, size, bitPos);
    bitPos++;
    
    // 简化实现：模拟宏块信息
    // 在实际应用中，需要根据 HEVC 标准解析具体的宏块数据
    
    // 假设一帧有 16x16 个宏块
    const int numMBs = 16 * 16;
    mbInfos.reserve(numMBs);
    
    for (int i = 0; i < numMBs; i++) {
        MacroblockInfo mbInfo;
        mbInfo.mb_addr = i;
        mbInfo.mb_type = i % 4; // 模拟不同的宏块类型
        mbInfo.pred_mode = i % 2; // 0: 帧内预测, 1: 帧间预测
        mbInfo.ref_idx_l0 = i % 3;
        mbInfo.ref_idx_l1 = i % 2;
        mbInfo.mv_l0[0] = (i - numMBs/2) * 2;
        mbInfo.mv_l0[1] = (i - numMBs/2) * 2;
        mbInfo.mv_l1[0] = (i - numMBs/2);
        mbInfo.mv_l1[1] = (i - numMBs/2);
        mbInfo.coded_block_pattern = i % 256;
        mbInfo.qp = 20 + (i % 10);
        
        mbInfos.push_back(mbInfo);
    }
    
    return true;
}

void MacroblockAnalyzer::printMacroblockInfo(const std::vector<MacroblockInfo>& mbInfos) {
    std::cout << "Macroblock Information:" << std::endl;
    std::cout << "  Total Macroblocks: " << mbInfos.size() << std::endl;
    
    // 只输出前几个宏块的信息，避免输出过多
    const int maxMBsToPrint = 10;
    int numMBsToPrint = std::min((int)mbInfos.size(), maxMBsToPrint);
    
    for (int i = 0; i < numMBsToPrint; i++) {
        const MacroblockInfo& mbInfo = mbInfos[i];
        std::cout << "  MB " << i << ":" << std::endl;
        std::cout << "    Address: " << mbInfo.mb_addr << std::endl;
        std::cout << "    Type: " << (int)mbInfo.mb_type << std::endl;
        std::cout << "    Pred Mode: " << (mbInfo.pred_mode == 0 ? "Intra" : "Inter") << std::endl;
        std::cout << "    RefIdx L0: " << (int)mbInfo.ref_idx_l0 << std::endl;
        std::cout << "    RefIdx L1: " << (int)mbInfo.ref_idx_l1 << std::endl;
        std::cout << "    MV L0: (" << mbInfo.mv_l0[0] << ", " << mbInfo.mv_l0[1] << ")" << std::endl;
        std::cout << "    MV L1: (" << mbInfo.mv_l1[0] << ", " << mbInfo.mv_l1[1] << ")" << std::endl;
        std::cout << "    Coded Block Pattern: " << (int)mbInfo.coded_block_pattern << std::endl;
        std::cout << "    QP: " << (int)mbInfo.qp << std::endl;
    }
    
    if (mbInfos.size() > maxMBsToPrint) {
        std::cout << "  ... and " << (mbInfos.size() - maxMBsToPrint) << " more macroblocks" << std::endl;
    }
    
    std::cout << std::endl;
}
