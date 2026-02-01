#include "vps_parser.h"
#include <iostream>

VPSParser::VPSParser() {
}

VPSParser::~VPSParser() {
}

uint8_t VPSParser::readBit(const uint8_t* data, size_t size, size_t bitPos) {
    size_t bytePos = bitPos / 8;
    size_t bitInByte = bitPos % 8;
    
    if (bytePos >= size) {
        return 0;
    }
    
    return (data[bytePos] >> (7 - bitInByte)) & 0x01;
}

uint32_t VPSParser::readBits(const uint8_t* data, size_t size, size_t& bitPos, uint8_t bitCount) {
    uint32_t result = 0;
    for (uint8_t i = 0; i < bitCount; i++) {
        result = (result << 1) | readBit(data, size, bitPos);
        bitPos++;
    }
    return result;
}

uint32_t VPSParser::parseExpGolomb(const uint8_t* data, size_t size, size_t& bitPos) {
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

bool VPSParser::parseVPS(const NALUnit& nalUnit, VPSInfo& vpsInfo) {
    if (nalUnit.nal_unit_type != NAL_UNIT_VPS) {
        return false;
    }
    
    const uint8_t* data = nalUnit.payload.data();
    size_t size = nalUnit.payload.size();
    size_t bitPos = 0;
    
    // 解析 VPS 信息
    // 1. vps_video_parameter_set_id
    vpsInfo.vps_video_parameter_set_id = parseExpGolomb(data, size, bitPos);
    
    // 2. vps_base_layer_internal_flag
    vpsInfo.vps_base_layer_internal_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 3. vps_base_layer_available_flag
    vpsInfo.vps_base_layer_available_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 4. vps_max_layers_minus1
    vpsInfo.vps_max_layers_minus1 = parseExpGolomb(data, size, bitPos);
    
    // 5. vps_max_sub_layers_minus1
    vpsInfo.vps_max_sub_layers_minus1 = parseExpGolomb(data, size, bitPos);
    
    // 6. vps_temporal_id_nesting_flag
    vpsInfo.vps_temporal_id_nesting_flag = readBit(data, size, bitPos);
    bitPos++;
    
    // 7. vps_reserved_0xffff_16bits
    vpsInfo.vps_reserved_0xffff_16bits = readBits(data, size, bitPos, 16);
    
    // 8. vps_reserved_0x7f_7bits
    vpsInfo.vps_reserved_0x7f_7bits = readBits(data, size, bitPos, 7);
    
    // 9. 解析子层信息
    uint8_t maxSubLayers = vpsInfo.vps_max_sub_layers_minus1 + 1;
    for (uint8_t i = 0; i < maxSubLayers; i++) {
        vpsInfo.vps_max_dec_pic_buffering_minus1[i] = parseExpGolomb(data, size, bitPos);
        vpsInfo.vps_max_num_reorder_pics[i] = parseExpGolomb(data, size, bitPos);
        vpsInfo.vps_max_latency_increase_plus1[i] = parseExpGolomb(data, size, bitPos);
    }
    
    // 10. vps_reserved_0x1f_5bits
    vpsInfo.vps_reserved_0x1f_5bits = readBits(data, size, bitPos, 5);
    
    // 11. vui_parameters_present_flag
    if (bitPos < size * 8) {
        vpsInfo.vui_parameters_present_flag = readBit(data, size, bitPos);
        bitPos++;
        
        // 解析 VUI 参数
        if (vpsInfo.vui_parameters_present_flag) {
            // 11.1 aspect_ratio_info_present_flag
            vpsInfo.vui_info.aspect_ratio_info_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (vpsInfo.vui_info.aspect_ratio_info_present_flag) {
                // 11.2 aspect_ratio_idc
                vpsInfo.vui_info.aspect_ratio_idc = readBits(data, size, bitPos, 8);
                
                // 11.3 自定义 SAR
                if (vpsInfo.vui_info.aspect_ratio_idc == 255) {
                    vpsInfo.vui_info.sar_width = readBits(data, size, bitPos, 16);
                    vpsInfo.vui_info.sar_height = readBits(data, size, bitPos, 16);
                }
            }
            
            // 11.4 overscan_info_present_flag
            vpsInfo.vui_info.overscan_info_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (vpsInfo.vui_info.overscan_info_present_flag) {
                // 11.5 overscan_appropriate_flag
                vpsInfo.vui_info.overscan_appropriate_flag = readBit(data, size, bitPos);
                bitPos++;
            }
            
            // 11.6 video_signal_type_present_flag
            vpsInfo.vui_info.video_signal_type_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (vpsInfo.vui_info.video_signal_type_present_flag) {
                // 11.7 video_format
                vpsInfo.vui_info.video_format = readBits(data, size, bitPos, 3);
                
                // 11.8 video_full_range_flag
                vpsInfo.vui_info.video_full_range_flag = readBit(data, size, bitPos);
                bitPos++;
                
                // 11.9 colour_description_present_flag
                vpsInfo.vui_info.colour_description_present_flag = readBit(data, size, bitPos);
                bitPos++;
                
                if (vpsInfo.vui_info.colour_description_present_flag) {
                    // 11.10 colour_primaries
                    vpsInfo.vui_info.colour_primaries = readBits(data, size, bitPos, 8);
                    
                    // 11.11 transfer_characteristics
                    vpsInfo.vui_info.transfer_characteristics = readBits(data, size, bitPos, 8);
                    
                    // 11.12 matrix_coefficients
                    vpsInfo.vui_info.matrix_coefficients = readBits(data, size, bitPos, 8);
                }
            }
            
            // 11.13 chroma_loc_info_present_flag
            vpsInfo.vui_info.chroma_loc_info_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (vpsInfo.vui_info.chroma_loc_info_present_flag) {
                // 11.14 chroma_sample_loc_type_top_field
                vpsInfo.vui_info.chroma_sample_loc_type_top_field = parseExpGolomb(data, size, bitPos);
                
                // 11.15 chroma_sample_loc_type_bottom_field
                vpsInfo.vui_info.chroma_sample_loc_type_bottom_field = parseExpGolomb(data, size, bitPos);
            }
            
            // 11.16 neutral_chroma_indication_flag
            vpsInfo.vui_info.neutral_chroma_indication_flag = readBit(data, size, bitPos);
            bitPos++;
            
            // 11.17 field_seq_flag
            vpsInfo.vui_info.field_seq_flag = readBit(data, size, bitPos);
            bitPos++;
            
            // 11.18 frame_field_info_present_flag
            vpsInfo.vui_info.frame_field_info_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            // 11.19 default_display_window_flag
            vpsInfo.vui_info.default_display_window_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (vpsInfo.vui_info.default_display_window_flag) {
                // 11.20 默认显示窗口偏移
                vpsInfo.vui_info.def_disp_win_left_offset = parseExpGolomb(data, size, bitPos);
                vpsInfo.vui_info.def_disp_win_right_offset = parseExpGolomb(data, size, bitPos);
                vpsInfo.vui_info.def_disp_win_top_offset = parseExpGolomb(data, size, bitPos);
                vpsInfo.vui_info.def_disp_win_bottom_offset = parseExpGolomb(data, size, bitPos);
            }
            
            // 11.21 vui_timing_info_present_flag
            vpsInfo.vui_info.vui_timing_info_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (vpsInfo.vui_info.vui_timing_info_present_flag) {
                // 11.22 vui_num_units_in_tick
                vpsInfo.vui_info.vui_num_units_in_tick = readBits(data, size, bitPos, 32);
                
                // 11.23 vui_time_scale
                vpsInfo.vui_info.vui_time_scale = readBits(data, size, bitPos, 32);
                
                // 11.24 vui_poc_proportional_to_timing_flag
                vpsInfo.vui_info.vui_poc_proportional_to_timing_flag = readBit(data, size, bitPos);
                bitPos++;
                
                if (vpsInfo.vui_info.vui_poc_proportional_to_timing_flag) {
                    // 11.25 vui_num_ticks_poc_diff_one_minus1
                    vpsInfo.vui_info.vui_num_ticks_poc_diff_one_minus1 = parseExpGolomb(data, size, bitPos);
                }
            }
            
            // 11.26 vui_hrd_parameters_present_flag
            vpsInfo.vui_info.vui_hrd_parameters_present_flag = readBit(data, size, bitPos);
            bitPos++;
            
            // 11.27 bitstream_restriction_flag
            vpsInfo.vui_info.bitstream_restriction_flag = readBit(data, size, bitPos);
            bitPos++;
            
            if (vpsInfo.vui_info.bitstream_restriction_flag) {
                // 11.28 比特流限制参数
                vpsInfo.vui_info.tiles_fixed_structure_flag = readBit(data, size, bitPos);
                bitPos++;
                
                vpsInfo.vui_info.motion_vectors_over_pic_boundaries_flag = readBit(data, size, bitPos);
                bitPos++;
                
                vpsInfo.vui_info.restricted_ref_pic_lists_flag = readBit(data, size, bitPos);
                bitPos++;
                
                vpsInfo.vui_info.min_spatial_segmentation_idc = parseExpGolomb(data, size, bitPos);
                vpsInfo.vui_info.max_bytes_per_pic_denom = parseExpGolomb(data, size, bitPos);
                vpsInfo.vui_info.max_bits_per_min_cu_denom = parseExpGolomb(data, size, bitPos);
                vpsInfo.vui_info.log2_max_mv_length_horizontal = parseExpGolomb(data, size, bitPos);
                vpsInfo.vui_info.log2_max_mv_length_vertical = parseExpGolomb(data, size, bitPos);
            }
        }
    } else {
        vpsInfo.vui_parameters_present_flag = 0;
    }
    
    // 12. vps_extension_flag
    if (bitPos < size * 8) {
        vpsInfo.vps_extension_flag = readBit(data, size, bitPos);
        bitPos++;
    } else {
        vpsInfo.vps_extension_flag = 0;
    }
    
    return true;
}

void VPSParser::printVPSInfo(const VPSInfo& vpsInfo) {
    std::cout << "VPS Information:" << std::endl;
    std::cout << "  Video Parameter Set ID: " << (int)vpsInfo.vps_video_parameter_set_id << std::endl;
    std::cout << "  Base Layer Internal Flag: " << (int)vpsInfo.vps_base_layer_internal_flag << std::endl;
    std::cout << "  Base Layer Available Flag: " << (int)vpsInfo.vps_base_layer_available_flag << std::endl;
    std::cout << "  Max Layers: " << (int)(vpsInfo.vps_max_layers_minus1 + 1) << std::endl;
    std::cout << "  Max Sub Layers: " << (int)(vpsInfo.vps_max_sub_layers_minus1 + 1) << std::endl;
    std::cout << "  Temporal ID Nesting Flag: " << (int)vpsInfo.vps_temporal_id_nesting_flag << std::endl;
    
    // 打印 VUI 参数信息
    if (vpsInfo.vui_parameters_present_flag) {
        std::cout << "\n  VUI Parameters:" << std::endl;
        
        // 打印宽高比信息
        if (vpsInfo.vui_info.aspect_ratio_info_present_flag) {
            std::cout << "    Aspect Ratio IDC: " << (int)vpsInfo.vui_info.aspect_ratio_idc << std::endl;
            if (vpsInfo.vui_info.aspect_ratio_idc == 255) {
                std::cout << "    SAR Width: " << vpsInfo.vui_info.sar_width << std::endl;
                std::cout << "    SAR Height: " << vpsInfo.vui_info.sar_height << std::endl;
            }
        }
        
        // 打印视频信号类型信息
        if (vpsInfo.vui_info.video_signal_type_present_flag) {
            std::cout << "    Video Format: " << (int)vpsInfo.vui_info.video_format << std::endl;
            std::cout << "    Full Range Flag: " << (int)vpsInfo.vui_info.video_full_range_flag << std::endl;
            
            if (vpsInfo.vui_info.colour_description_present_flag) {
                std::cout << "    Colour Primaries: " << (int)vpsInfo.vui_info.colour_primaries << std::endl;
                std::cout << "    Transfer Characteristics: " << (int)vpsInfo.vui_info.transfer_characteristics << std::endl;
                std::cout << "    Matrix Coefficients: " << (int)vpsInfo.vui_info.matrix_coefficients << std::endl;
            }
        }
        
        // 打印色度位置信息
        if (vpsInfo.vui_info.chroma_loc_info_present_flag) {
            std::cout << "    Chroma Sample Loc Type Top Field: " << (int)vpsInfo.vui_info.chroma_sample_loc_type_top_field << std::endl;
            std::cout << "    Chroma Sample Loc Type Bottom Field: " << (int)vpsInfo.vui_info.chroma_sample_loc_type_bottom_field << std::endl;
        }
        
        // 打印时序信息
        if (vpsInfo.vui_info.vui_timing_info_present_flag) {
            std::cout << "    Num Units In Tick: " << vpsInfo.vui_info.vui_num_units_in_tick << std::endl;
            std::cout << "    Time Scale: " << vpsInfo.vui_info.vui_time_scale << std::endl;
            std::cout << "    Poc Proportional To Timing Flag: " << (int)vpsInfo.vui_info.vui_poc_proportional_to_timing_flag << std::endl;
        }
        
        // 打印比特流限制信息
        if (vpsInfo.vui_info.bitstream_restriction_flag) {
            std::cout << "    Tiles Fixed Structure Flag: " << (int)vpsInfo.vui_info.tiles_fixed_structure_flag << std::endl;
            std::cout << "    Motion Vectors Over Pic Boundaries Flag: " << (int)vpsInfo.vui_info.motion_vectors_over_pic_boundaries_flag << std::endl;
            std::cout << "    Restricted Ref Pic Lists Flag: " << (int)vpsInfo.vui_info.restricted_ref_pic_lists_flag << std::endl;
        }
    }
    
    std::cout << std::endl;
}
