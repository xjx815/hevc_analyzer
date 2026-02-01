#include "pps_parser.h"
#include <iostream>

PPSParser::PPSParser() {
}

PPSParser::~PPSParser() {
}

uint8_t PPSParser::readBit(const uint8_t* data, size_t size, size_t bitPos) {
    size_t bytePos = bitPos / 8;
    size_t bitInByte = bitPos % 8;
    
    if (bytePos >= size) {
        return 0;
    }
    
    return (data[bytePos] >> (7 - bitInByte)) & 0x01;
}

uint32_t PPSParser::readBits(const uint8_t* data, size_t size, size_t& bitPos, uint8_t bitCount) {
    uint32_t result = 0;
    for (uint8_t i = 0; i < bitCount; i++) {
        result = (result << 1) | readBit(data, size, bitPos);
        bitPos++;
    }
    return result;
}

uint32_t PPSParser::parseExpGolomb(const uint8_t* data, size_t size, size_t& bitPos) {
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

bool PPSParser::parsePPS(const NALUnit& nalUnit, PPSInfo& ppsInfo) {
    if (nalUnit.nal_unit_type != NAL_UNIT_PPS) {
        return false;
    }
    
    const uint8_t* data = nalUnit.payload.data();
    size_t size = nalUnit.payload.size();
    size_t bitPos = 0;
    
    // 解析 PPS 信息
    // 1. pps_pic_parameter_set_id
    ppsInfo.pps_pic_parameter_set_id = parseExpGolomb(data, size, bitPos);
    
    // 2. pps_seq_parameter_set_id
    ppsInfo.pps_seq_parameter_set_id = parseExpGolomb(data, size, bitPos);
    
    // 3. dependent_slice_segments_enabled_flag
    ppsInfo.dependent_slice_segments_enabled_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 4. output_flag_present_flag
    ppsInfo.output_flag_present_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 5. num_extra_slice_header_bits
    ppsInfo.num_extra_slice_header_bits = parseExpGolomb(data, size, bitPos);
    
    // 6. pps_num_ref_idx_l0_default_active_minus1
    ppsInfo.pps_num_ref_idx_l0_default_active_minus1 = parseExpGolomb(data, size, bitPos);
    
    // 7. pps_num_ref_idx_l1_default_active_minus1
    ppsInfo.pps_num_ref_idx_l1_default_active_minus1 = parseExpGolomb(data, size, bitPos);
    
    // 8. pps_init_qp_minus26
    uint32_t initQp = parseExpGolomb(data, size, bitPos);
    ppsInfo.pps_init_qp_minus26 = initQp - 26;
    
    // 9. pps_init_qs_minus26
    uint32_t initQs = parseExpGolomb(data, size, bitPos);
    ppsInfo.pps_init_qs_minus26 = initQs - 26;
    
    // 10. pps_chroma_qp_index_offset
    int32_t chromaQpOffset = parseExpGolomb(data, size, bitPos);
    if (chromaQpOffset % 2 == 0) {
        ppsInfo.pps_chroma_qp_index_offset = -(chromaQpOffset / 2);
    } else {
        ppsInfo.pps_chroma_qp_index_offset = (chromaQpOffset + 1) / 2;
    }
    
    // 11. pps_slice_chroma_qp_offsets_present_flag
    ppsInfo.pps_slice_chroma_qp_offsets_present_flag = readBit(data, size, bitPos);
    bitPos++;
    
    if (ppsInfo.pps_slice_chroma_qp_offsets_present_flag) {
        // 12. pps_cb_qp_offset
        int32_t cbQpOffset = parseExpGolomb(data, size, bitPos);
        if (cbQpOffset % 2 == 0) {
            ppsInfo.pps_cb_qp_offset = -(cbQpOffset / 2);
        } else {
            ppsInfo.pps_cb_qp_offset = (cbQpOffset + 1) / 2;
        }
        
        // 13. pps_cr_qp_offset
        int32_t crQpOffset = parseExpGolomb(data, size, bitPos);
        if (crQpOffset % 2 == 0) {
            ppsInfo.pps_cr_qp_offset = -(crQpOffset / 2);
        } else {
            ppsInfo.pps_cr_qp_offset = (crQpOffset + 1) / 2;
        }
    }
    
    // 14. pps_deblocking_filter_control_present_flag
    ppsInfo.pps_deblocking_filter_control_present_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 15. pps_constrained_intra_pred_flag
    ppsInfo.pps_constrained_intra_pred_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 16. pps_transform_skip_enabled_flag
    ppsInfo.pps_transform_skip_enabled_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 17. pps_amvp_enabled_flag
    ppsInfo.pps_amvp_enabled_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 18. pps_sao_enabled_flag
    ppsInfo.pps_sao_enabled_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 19. pps_deblocking_filter_control_present_flag
    if (ppsInfo.pps_deblocking_filter_control_present_flag) {
        // 20. pps_luma_beta_offset_div2
        int32_t betaOffset = parseExpGolomb(data, size, bitPos);
        ppsInfo.pps_luma_beta_offset_div2 = betaOffset - 12;
        
        // 21. pps_luma_tc_offset_div2
        int32_t tcOffset = parseExpGolomb(data, size, bitPos);
        ppsInfo.pps_luma_tc_offset_div2 = tcOffset - 12;
        
        // 22. pps_chroma_beta_offset_div2
        int32_t chromaBetaOffset = parseExpGolomb(data, size, bitPos);
        ppsInfo.pps_chroma_beta_offset_div2 = chromaBetaOffset - 12;
        
        // 23. pps_chroma_tc_offset_div2
        int32_t chromaTcOffset = parseExpGolomb(data, size, bitPos);
        ppsInfo.pps_chroma_tc_offset_div2 = chromaTcOffset - 12;
    }
    
    // 24. pps_num_tile_columns_minus1
    ppsInfo.pps_num_tile_columns_minus1 = parseExpGolomb(data, size, bitPos);
    
    // 25. pps_num_tile_rows_minus1
    ppsInfo.pps_num_tile_rows_minus1 = parseExpGolomb(data, size, bitPos);
    
    // 26. pps_loop_filter_across_tiles_enabled_flag
    ppsInfo.pps_loop_filter_across_tiles_enabled_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 27. pps_sample_adaptive_offset_enabled_flag
    ppsInfo.pps_sample_adaptive_offset_enabled_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 28. pps_pic_parameter_set_id_extension_flag
    if (bitPos < size * 8) {
        ppsInfo.pps_pic_parameter_set_id_extension_flag = readBit(data, size, bitPos);
        bitPos++;
    } else {
        ppsInfo.pps_pic_parameter_set_id_extension_flag = 0;
    }
    
    return true;
}

void PPSParser::printPPSInfo(const PPSInfo& ppsInfo) {
    std::cout << "PPS Information:" << std::endl;
    std::cout << "  Pic Parameter Set ID: " << (int)ppsInfo.pps_pic_parameter_set_id << std::endl;
    std::cout << "  Seq Parameter Set ID: " << (int)ppsInfo.pps_seq_parameter_set_id << std::endl;
    std::cout << "  Dependent Slice Segments Enabled: " << (int)ppsInfo.dependent_slice_segments_enabled_flag << std::endl;
    std::cout << "  Output Flag Present: " << (int)ppsInfo.output_flag_present_flag << std::endl;
    std::cout << "  Num Extra Slice Header Bits: " << (int)ppsInfo.num_extra_slice_header_bits << std::endl;
    std::cout << "  Num RefIdx L0 Default Active: " << (int)(ppsInfo.pps_num_ref_idx_l0_default_active_minus1 + 1) << std::endl;
    std::cout << "  Num RefIdx L1 Default Active: " << (int)(ppsInfo.pps_num_ref_idx_l1_default_active_minus1 + 1) << std::endl;
    std::cout << "  Init QP: " << (int)(ppsInfo.pps_init_qp_minus26 + 26) << std::endl;
    std::cout << "  Init QS: " << (int)(ppsInfo.pps_init_qs_minus26 + 26) << std::endl;
    std::cout << "  Chroma QP Index Offset: " << (int)ppsInfo.pps_chroma_qp_index_offset << std::endl;
    std::cout << "  Slice Chroma QP Offsets Present: " << (int)ppsInfo.pps_slice_chroma_qp_offsets_present_flag << std::endl;
    if (ppsInfo.pps_slice_chroma_qp_offsets_present_flag) {
        std::cout << "  CB QP Offset: " << (int)ppsInfo.pps_cb_qp_offset << std::endl;
        std::cout << "  CR QP Offset: " << (int)ppsInfo.pps_cr_qp_offset << std::endl;
    }
    std::cout << "  Deblocking Filter Control Present: " << (int)ppsInfo.pps_deblocking_filter_control_present_flag << std::endl;
    std::cout << "  Constrained Intra Pred Flag: " << (int)ppsInfo.pps_constrained_intra_pred_flag << std::endl;
    std::cout << "  Transform Skip Enabled Flag: " << (int)ppsInfo.pps_transform_skip_enabled_flag << std::endl;
    std::cout << "  AMVP Enabled Flag: " << (int)ppsInfo.pps_amvp_enabled_flag << std::endl;
    std::cout << "  SAO Enabled Flag: " << (int)ppsInfo.pps_sao_enabled_flag << std::endl;
    std::cout << "  Num Tile Columns: " << (int)(ppsInfo.pps_num_tile_columns_minus1 + 1) << std::endl;
    std::cout << "  Num Tile Rows: " << (int)(ppsInfo.pps_num_tile_rows_minus1 + 1) << std::endl;
    std::cout << "  Loop Filter Across Tiles Enabled: " << (int)ppsInfo.pps_loop_filter_across_tiles_enabled_flag << std::endl;
    std::cout << "  Sample Adaptive Offset Enabled: " << (int)ppsInfo.pps_sample_adaptive_offset_enabled_flag << std::endl;
    std::cout << std::endl;
}
