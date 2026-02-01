#include "sps_parser.h"
#include <iostream>

SPSParser::SPSParser() {
}

SPSParser::~SPSParser() {
}

uint8_t SPSParser::readBit(const uint8_t* data, size_t size, size_t bitPos) {
    size_t bytePos = bitPos / 8;
    size_t bitInByte = bitPos % 8;
    
    if (bytePos >= size) {
        return 0;
    }
    
    return (data[bytePos] >> (7 - bitInByte)) & 0x01;
}

uint32_t SPSParser::readBits(const uint8_t* data, size_t size, size_t& bitPos, uint8_t bitCount) {
    uint32_t result = 0;
    for (uint8_t i = 0; i < bitCount; i++) {
        result = (result << 1) | readBit(data, size, bitPos);
        bitPos++;
    }
    return result;
}

uint32_t SPSParser::parseExpGolomb(const uint8_t* data, size_t size, size_t& bitPos) {
    // 计算前导零的个数
    uint8_t leadingZeros = 0;
    while (leadingZeros < 32 && bitPos < size * 8) {
        if (readBit(data, size, bitPos) == 0) {
            leadingZeros++;
            bitPos++;
        } else {
            break;
        }
    }
    
    // 跳过第一个 1
    if (bitPos < size * 8) {
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

bool SPSParser::parseSPS(const NALUnit& nalUnit, SPSInfo& spsInfo) {
    if (nalUnit.nal_unit_type != NAL_UNIT_SPS) {
        return false;
    }
    
    const uint8_t* data = nalUnit.payload.data();
    size_t size = nalUnit.payload.size();
    size_t bitPos = 0;
    
    // 解析 SPS 信息
    // 1. sps_video_parameter_set_id
    spsInfo.sps_video_parameter_set_id = parseExpGolomb(data, size, bitPos);
    
    // 2. sps_max_sub_layers_minus1
    spsInfo.sps_max_sub_layers_minus1 = parseExpGolomb(data, size, bitPos);
    
    // 3. sps_tier_flag
    spsInfo.sps_tier_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 4. sps_profile_idc
    spsInfo.sps_profile_idc = readBits(data, size, bitPos, 8);
    
    // 5. sps_profile_compatibility_flag
    for (int i = 0; i < 32; i++) {
        spsInfo.sps_profile_compatibility_flag[i] = readBit(data, size, bitPos);
        bitPos++;
    }
    
    // 6. sps_progressive_source_flag
    spsInfo.sps_progressive_source_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 7. sps_interlaced_source_flag
    spsInfo.sps_interlaced_source_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 8. sps_non_packed_constraint_flag
    spsInfo.sps_non_packed_constraint_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 9. sps_frame_only_constraint_flag
    spsInfo.sps_frame_only_constraint_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 10. 其他约束标志
    spsInfo.sps_max_12bit_constraint_flag = readBit(data, size, bitPos);
    bitPos++;
    spsInfo.sps_max_10bit_constraint_flag = readBit(data, size, bitPos);
    bitPos++;
    spsInfo.sps_max_8bit_constraint_flag = readBit(data, size, bitPos);
    bitPos++;
    spsInfo.sps_max_422chroma_constraint_flag = readBit(data, size, bitPos);
    bitPos++;
    spsInfo.sps_max_420chroma_constraint_flag = readBit(data, size, bitPos);
    bitPos++;
    spsInfo.sps_max_monochrome_constraint_flag = readBit(data, size, bitPos);
    bitPos++;
    spsInfo.sps_int_max_wdt_constraint_flag = readBit(data, size, bitPos);
    bitPos++;
    spsInfo.sps_int_max_hgt_constraint_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 11. 解析子层信息
    spsInfo.sps_sub_layer_ordering_info_present_flag = readBit(data, size, bitPos);
    bitPos++;
    
    uint8_t maxSubLayers = spsInfo.sps_max_sub_layers_minus1 + 1;
    for (uint8_t i = 0; i < maxSubLayers; i++) {
        spsInfo.sps_max_dec_pic_buffering_minus1[i] = parseExpGolomb(data, size, bitPos);
        spsInfo.sps_max_num_reorder_pics[i] = parseExpGolomb(data, size, bitPos);
        spsInfo.sps_max_latency_increase_plus1[i] = parseExpGolomb(data, size, bitPos);
    }
    
    // 12. sps_max_frame_num_minus4
    spsInfo.sps_max_frame_num_minus4 = parseExpGolomb(data, size, bitPos);
    
    // 13. log2_max_pic_order_cnt_lsb_minus4
    spsInfo.log2_max_pic_order_cnt_lsb_minus4 = parseExpGolomb(data, size, bitPos);
    
    // 14. sps_num_ref_frames_in_pic_order_cnt_cycle
    spsInfo.sps_num_ref_frames_in_pic_order_cnt_cycle = parseExpGolomb(data, size, bitPos);
    
    // 15. 解析偏移值
    for (uint8_t i = 0; i < spsInfo.sps_num_ref_frames_in_pic_order_cnt_cycle; i++) {
        spsInfo.sps_offset_for_ref_frame[i] = parseExpGolomb(data, size, bitPos);
        if (spsInfo.sps_offset_for_ref_frame[i] % 2 == 0) {
            spsInfo.sps_offset_for_ref_frame[i] = -(spsInfo.sps_offset_for_ref_frame[i] / 2);
        } else {
            spsInfo.sps_offset_for_ref_frame[i] = (spsInfo.sps_offset_for_ref_frame[i] + 1) / 2;
        }
    }
    
    // 16. sps_max_num_ref_frames
    spsInfo.sps_max_num_ref_frames = parseExpGolomb(data, size, bitPos);
    
    // 17. sps_gaps_in_frame_num_value_allowed_flag
    spsInfo.sps_gaps_in_frame_num_value_allowed_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 18. pic_width_in_luma_samples
    uint32_t pic_width_in_luma_samples_minus1 = parseExpGolomb(data, size, bitPos);
    spsInfo.pic_width_in_luma_samples = pic_width_in_luma_samples_minus1 + 1;
    
    // 19. pic_height_in_luma_samples
    uint32_t pic_height_in_luma_samples_minus1 = parseExpGolomb(data, size, bitPos);
    spsInfo.pic_height_in_luma_samples = pic_height_in_luma_samples_minus1 + 1;
    
    // 20. sps_conformance_window_flag
    spsInfo.sps_conformance_window_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 21. 解析窗口偏移
    if (spsInfo.sps_conformance_window_flag) {
        spsInfo.conf_win_left_offset = parseExpGolomb(data, size, bitPos);
        spsInfo.conf_win_right_offset = parseExpGolomb(data, size, bitPos);
        spsInfo.conf_win_top_offset = parseExpGolomb(data, size, bitPos);
        spsInfo.conf_win_bottom_offset = parseExpGolomb(data, size, bitPos);
    }
    
    // 22. sps_btt_flag
    spsInfo.sps_btt_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 23. sps_btp_flag
    spsInfo.sps_btp_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 24. sps_dpb_output_delay_handling_flag
    spsInfo.sps_dpb_output_delay_handling_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 25. sps_level_idc
    spsInfo.sps_level_idc = readBits(data, size, bitPos, 8);
    
    // 26. sps_seq_parameter_set_id
    spsInfo.sps_seq_parameter_set_id = parseExpGolomb(data, size, bitPos);
    
    // 27. vui_parameters_present_flag
    if (bitPos < size * 8) {
        spsInfo.vui_parameters_present_flag = readBit(data, size, bitPos);
        bitPos++;
        
        // 解析 VUI 参数
        if (spsInfo.vui_parameters_present_flag) {
            // 27.1 aspect_ratio_info_present_flag
            spsInfo.vui_info.aspect_ratio_info_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (spsInfo.vui_info.aspect_ratio_info_present_flag) {
                // 27.2 aspect_ratio_idc
                spsInfo.vui_info.aspect_ratio_idc = readBits(data, size, bitPos, 8);
                
                // 27.3 自定义 SAR
                if (spsInfo.vui_info.aspect_ratio_idc == 255) {
                    spsInfo.vui_info.sar_width = readBits(data, size, bitPos, 16);
                    spsInfo.vui_info.sar_height = readBits(data, size, bitPos, 16);
                }
            }
            
            // 27.4 overscan_info_present_flag
            spsInfo.vui_info.overscan_info_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (spsInfo.vui_info.overscan_info_present_flag) {
                // 27.5 overscan_appropriate_flag
                spsInfo.vui_info.overscan_appropriate_flag = readBit(data, size, bitPos);
                bitPos++;
            }
            
            // 27.6 video_signal_type_present_flag
            spsInfo.vui_info.video_signal_type_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (spsInfo.vui_info.video_signal_type_present_flag) {
                // 27.7 video_format
                spsInfo.vui_info.video_format = readBits(data, size, bitPos, 3);
                
                // 27.8 video_full_range_flag
                spsInfo.vui_info.video_full_range_flag = readBit(data, size, bitPos);
                bitPos++;
                
                // 27.9 colour_description_present_flag
                spsInfo.vui_info.colour_description_present_flag = readBit(data, size, bitPos);
                bitPos++;
                
                if (spsInfo.vui_info.colour_description_present_flag) {
                    // 27.10 colour_primaries
                    spsInfo.vui_info.colour_primaries = readBits(data, size, bitPos, 8);
                    
                    // 27.11 transfer_characteristics
                    spsInfo.vui_info.transfer_characteristics = readBits(data, size, bitPos, 8);
                    
                    // 27.12 matrix_coefficients
                    spsInfo.vui_info.matrix_coefficients = readBits(data, size, bitPos, 8);
                }
            }
            
            // 27.13 chroma_loc_info_present_flag
            spsInfo.vui_info.chroma_loc_info_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (spsInfo.vui_info.chroma_loc_info_present_flag) {
                // 27.14 chroma_sample_loc_type_top_field
                spsInfo.vui_info.chroma_sample_loc_type_top_field = parseExpGolomb(data, size, bitPos);
                
                // 27.15 chroma_sample_loc_type_bottom_field
                spsInfo.vui_info.chroma_sample_loc_type_bottom_field = parseExpGolomb(data, size, bitPos);
            }
            
            // 27.16 neutral_chroma_indication_flag
            spsInfo.vui_info.neutral_chroma_indication_flag = readBit(data, size, bitPos);
            bitPos++;
            
            // 27.17 field_seq_flag
            spsInfo.vui_info.field_seq_flag = readBit(data, size, bitPos);
            bitPos++;
            
            // 27.18 frame_field_info_present_flag
            spsInfo.vui_info.frame_field_info_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            // 27.19 default_display_window_flag
            spsInfo.vui_info.default_display_window_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (spsInfo.vui_info.default_display_window_flag) {
                // 27.20 默认显示窗口偏移
                spsInfo.vui_info.def_disp_win_left_offset = parseExpGolomb(data, size, bitPos);
                spsInfo.vui_info.def_disp_win_right_offset = parseExpGolomb(data, size, bitPos);
                spsInfo.vui_info.def_disp_win_top_offset = parseExpGolomb(data, size, bitPos);
                spsInfo.vui_info.def_disp_win_bottom_offset = parseExpGolomb(data, size, bitPos);
            }
            
            // 27.21 vui_timing_info_present_flag
            spsInfo.vui_info.vui_timing_info_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (spsInfo.vui_info.vui_timing_info_present_flag) {
                // 27.22 vui_num_units_in_tick
                spsInfo.vui_info.vui_num_units_in_tick = readBits(data, size, bitPos, 32);
                
                // 27.23 vui_time_scale
                spsInfo.vui_info.vui_time_scale = readBits(data, size, bitPos, 32);
                
                // 27.24 vui_poc_proportional_to_timing_flag
                spsInfo.vui_info.vui_poc_proportional_to_timing_flag = readBit(data, size, bitPos);
                bitPos++;
                
                if (spsInfo.vui_info.vui_poc_proportional_to_timing_flag) {
                    // 27.25 vui_num_ticks_poc_diff_one_minus1
                    spsInfo.vui_info.vui_num_ticks_poc_diff_one_minus1 = parseExpGolomb(data, size, bitPos);
                }
            }
            
            // 27.26 vui_hrd_parameters_present_flag
            spsInfo.vui_info.vui_hrd_parameters_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            // 27.27 bitstream_restriction_flag
            spsInfo.vui_info.bitstream_restriction_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (spsInfo.vui_info.bitstream_restriction_flag) {
                // 27.28 比特流限制参数
                spsInfo.vui_info.tiles_fixed_structure_flag = readBit(data, size, bitPos);
                bitPos++;
                
                spsInfo.vui_info.motion_vectors_over_pic_boundaries_flag = readBit(data, size, bitPos);
                bitPos++;
                
                spsInfo.vui_info.restricted_ref_pic_lists_flag = readBit(data, size, bitPos);
                bitPos++;
                
                spsInfo.vui_info.min_spatial_segmentation_idc = parseExpGolomb(data, size, bitPos);
                spsInfo.vui_info.max_bytes_per_pic_denom = parseExpGolomb(data, size, bitPos);
                spsInfo.vui_info.max_bits_per_min_cu_denom = parseExpGolomb(data, size, bitPos);
                spsInfo.vui_info.log2_max_mv_length_horizontal = parseExpGolomb(data, size, bitPos);
                spsInfo.vui_info.log2_max_mv_length_vertical = parseExpGolomb(data, size, bitPos);
            }
        }
    } else {
        spsInfo.vui_parameters_present_flag = 0;
    }
    
    // 28. sps_extension_flag
    if (bitPos < size * 8) {
        spsInfo.sps_extension_flag = readBit(data, size, bitPos);
        bitPos++;
    } else {
        spsInfo.sps_extension_flag = 0;
    }
    
    return true;
}

void SPSParser::printSPSInfo(const SPSInfo& spsInfo) {
    std::cout << "SPS Information:" << std::endl;
    std::cout << "  Video Parameter Set ID: " << (int)spsInfo.sps_video_parameter_set_id << std::endl;
    std::cout << "  Max Sub Layers: " << (int)(spsInfo.sps_max_sub_layers_minus1 + 1) << std::endl;
    std::cout << "  Tier Flag: " << (int)spsInfo.sps_tier_flag << std::endl;
    std::cout << "  Profile IDC: " << (int)spsInfo.sps_profile_idc << std::endl;
    std::cout << "  Progressive Source Flag: " << (int)spsInfo.sps_progressive_source_flag << std::endl;
    std::cout << "  Interlaced Source Flag: " << (int)spsInfo.sps_interlaced_source_flag << std::endl;
    std::cout << "  Frame Only Constraint Flag: " << (int)spsInfo.sps_frame_only_constraint_flag << std::endl;
    std::cout << "  Max Frame Num: " << (int)(spsInfo.sps_max_frame_num_minus4 + 4) << std::endl;
    std::cout << "  Log2 Max Pic Order Cnt LSB: " << (int)(spsInfo.log2_max_pic_order_cnt_lsb_minus4 + 4) << std::endl;
    std::cout << "  Max Num Ref Frames: " << (int)spsInfo.sps_max_num_ref_frames << std::endl;
    std::cout << "  Picture Width: " << spsInfo.pic_width_in_luma_samples << " pixels" << std::endl;
    std::cout << "  Picture Height: " << spsInfo.pic_height_in_luma_samples << " pixels" << std::endl;
    std::cout << "  Level IDC: " << (int)spsInfo.sps_level_idc << std::endl;
    std::cout << "  Seq Parameter Set ID: " << (int)spsInfo.sps_seq_parameter_set_id << std::endl;
    
    // 打印 VUI 参数信息
    if (spsInfo.vui_parameters_present_flag) {
        std::cout << "\n  VUI Parameters:" << std::endl;
        
        // 打印宽高比信息
        if (spsInfo.vui_info.aspect_ratio_info_present_flag) {
            std::cout << "    Aspect Ratio IDC: " << (int)spsInfo.vui_info.aspect_ratio_idc << std::endl;
            if (spsInfo.vui_info.aspect_ratio_idc == 255) {
                std::cout << "    SAR Width: " << spsInfo.vui_info.sar_width << std::endl;
                std::cout << "    SAR Height: " << spsInfo.vui_info.sar_height << std::endl;
            }
        }
        
        // 打印视频信号类型信息
        if (spsInfo.vui_info.video_signal_type_present_flag) {
            std::cout << "    Video Format: " << (int)spsInfo.vui_info.video_format << std::endl;
            std::cout << "    Full Range Flag: " << (int)spsInfo.vui_info.video_full_range_flag << std::endl;
            
            if (spsInfo.vui_info.colour_description_present_flag) {
                std::cout << "    Colour Primaries: " << (int)spsInfo.vui_info.colour_primaries << std::endl;
                std::cout << "    Transfer Characteristics: " << (int)spsInfo.vui_info.transfer_characteristics << std::endl;
                std::cout << "    Matrix Coefficients: " << (int)spsInfo.vui_info.matrix_coefficients << std::endl;
            }
        }
        
        // 打印色度位置信息
        if (spsInfo.vui_info.chroma_loc_info_present_flag) {
            std::cout << "    Chroma Sample Loc Type Top Field: " << (int)spsInfo.vui_info.chroma_sample_loc_type_top_field << std::endl;
            std::cout << "    Chroma Sample Loc Type Bottom Field: " << (int)spsInfo.vui_info.chroma_sample_loc_type_bottom_field << std::endl;
        }
        
        // 打印时序信息
        if (spsInfo.vui_info.vui_timing_info_present_flag) {
            std::cout << "    Num Units In Tick: " << spsInfo.vui_info.vui_num_units_in_tick << std::endl;
            std::cout << "    Time Scale: " << spsInfo.vui_info.vui_time_scale << std::endl;
            std::cout << "    Poc Proportional To Timing Flag: " << (int)spsInfo.vui_info.vui_poc_proportional_to_timing_flag << std::endl;
        }
        
        // 打印比特流限制信息
        if (spsInfo.vui_info.bitstream_restriction_flag) {
            std::cout << "    Tiles Fixed Structure Flag: " << (int)spsInfo.vui_info.tiles_fixed_structure_flag << std::endl;
            std::cout << "    Motion Vectors Over Pic Boundaries Flag: " << (int)spsInfo.vui_info.motion_vectors_over_pic_boundaries_flag << std::endl;
            std::cout << "    Restricted Ref Pic Lists Flag: " << (int)spsInfo.vui_info.restricted_ref_pic_lists_flag << std::endl;
        }
    }
    
    std::cout << std::endl;
}
