#include "vps_parser.h"
#include "bit_stream_reader.h"
#include <iostream>

VPSParser::VPSParser() {
}

VPSParser::~VPSParser() {
}

bool VPSParser::parseVPS(const NALUnit& nalUnit, VPSInfo& vpsInfo) {
    if (nalUnit.nal_unit_type != NAL_UNIT_VPS) {
        return false;
    }

    const uint8_t* data = nalUnit.payload.data();
    size_t size = nalUnit.payload.size();
    BitStreamReader reader(data, size);

    // 1. vps_video_parameter_set_id — u(4) per HEVC spec
    vpsInfo.vps_video_parameter_set_id = reader.readBits(4);

    // 2. vps_base_layer_internal_flag — u(1)
    vpsInfo.vps_base_layer_internal_flag = reader.readBit();

    // 3. vps_base_layer_available_flag — u(1)
    vpsInfo.vps_base_layer_available_flag = reader.readBit();

    // 4. vps_max_layers_minus1 — u(6) per HEVC spec
    vpsInfo.vps_max_layers_minus1 = reader.readBits(6);

    // 5. vps_max_sub_layers_minus1 — u(3) per HEVC spec
    vpsInfo.vps_max_sub_layers_minus1 = reader.readBits(3);

    // 6. vps_temporal_id_nesting_flag — u(1)
    vpsInfo.vps_temporal_id_nesting_flag = reader.readBit();

    // 7. vps_reserved_0xffff_16bits — u(16)
    vpsInfo.vps_reserved_0xffff_16bits = reader.readBits(16);

    // 8. profile_tier_level for each layer (simplified: base layer only)
    // vps_reserved_0x7f_7bits is actually part of profile_tier_level
    vpsInfo.vps_reserved_0x7f_7bits = reader.readBits(7);

    // 9. Sub-layer DPB parameters
    uint8_t maxSubLayers = vpsInfo.vps_max_sub_layers_minus1 + 1;
    for (uint8_t i = 0; i < maxSubLayers; i++) {
        vpsInfo.vps_max_dec_pic_buffering_minus1[i] = reader.readUE();
        vpsInfo.vps_max_num_reorder_pics[i] = reader.readUE();
        vpsInfo.vps_max_latency_increase_plus1[i] = reader.readUE();
    }

    // 10. vps_reserved_0x1f_5bits — u(5)
    vpsInfo.vps_reserved_0x1f_5bits = reader.readBits(5);

    // 11. vui_parameters_present_flag — u(1)
    if (reader.hasMoreBits()) {
        vpsInfo.vui_parameters_present_flag = reader.readBit();

        if (vpsInfo.vui_parameters_present_flag) {
            // 11.1 aspect_ratio_info_present_flag
            vpsInfo.vui_info.aspect_ratio_info_present_flag = reader.readBit();

            if (vpsInfo.vui_info.aspect_ratio_info_present_flag) {
                vpsInfo.vui_info.aspect_ratio_idc = reader.readBits(8);

                if (vpsInfo.vui_info.aspect_ratio_idc == 255) {
                    vpsInfo.vui_info.sar_width = reader.readBits(16);
                    vpsInfo.vui_info.sar_height = reader.readBits(16);
                }
            }

            vpsInfo.vui_info.overscan_info_present_flag = reader.readBit();
            if (vpsInfo.vui_info.overscan_info_present_flag) {
                vpsInfo.vui_info.overscan_appropriate_flag = reader.readBit();
            }

            vpsInfo.vui_info.video_signal_type_present_flag = reader.readBit();
            if (vpsInfo.vui_info.video_signal_type_present_flag) {
                vpsInfo.vui_info.video_format = reader.readBits(3);
                vpsInfo.vui_info.video_full_range_flag = reader.readBit();
                vpsInfo.vui_info.colour_description_present_flag = reader.readBit();

                if (vpsInfo.vui_info.colour_description_present_flag) {
                    vpsInfo.vui_info.colour_primaries = reader.readBits(8);
                    vpsInfo.vui_info.transfer_characteristics = reader.readBits(8);
                    vpsInfo.vui_info.matrix_coefficients = reader.readBits(8);
                }
            }

            vpsInfo.vui_info.chroma_loc_info_present_flag = reader.readBit();
            if (vpsInfo.vui_info.chroma_loc_info_present_flag) {
                vpsInfo.vui_info.chroma_sample_loc_type_top_field = reader.readUE();
                vpsInfo.vui_info.chroma_sample_loc_type_bottom_field = reader.readUE();
            }

            vpsInfo.vui_info.neutral_chroma_indication_flag = reader.readBit();
            vpsInfo.vui_info.field_seq_flag = reader.readBit();
            vpsInfo.vui_info.frame_field_info_present_flag = reader.readBit();

            vpsInfo.vui_info.default_display_window_flag = reader.readBit();
            if (vpsInfo.vui_info.default_display_window_flag) {
                vpsInfo.vui_info.def_disp_win_left_offset = reader.readUE();
                vpsInfo.vui_info.def_disp_win_right_offset = reader.readUE();
                vpsInfo.vui_info.def_disp_win_top_offset = reader.readUE();
                vpsInfo.vui_info.def_disp_win_bottom_offset = reader.readUE();
            }

            vpsInfo.vui_info.vui_timing_info_present_flag = reader.readBit();
            if (vpsInfo.vui_info.vui_timing_info_present_flag) {
                vpsInfo.vui_info.vui_num_units_in_tick = reader.readBits(32);
                vpsInfo.vui_info.vui_time_scale = reader.readBits(32);
                vpsInfo.vui_info.vui_poc_proportional_to_timing_flag = reader.readBit();

                if (vpsInfo.vui_info.vui_poc_proportional_to_timing_flag) {
                    vpsInfo.vui_info.vui_num_ticks_poc_diff_one_minus1 = reader.readUE();
                }
            }

            vpsInfo.vui_info.vui_hrd_parameters_present_flag = reader.readBit();
            vpsInfo.vui_info.bitstream_restriction_flag = reader.readBit();

            if (vpsInfo.vui_info.bitstream_restriction_flag) {
                vpsInfo.vui_info.tiles_fixed_structure_flag = reader.readBit();
                vpsInfo.vui_info.motion_vectors_over_pic_boundaries_flag = reader.readBit();
                vpsInfo.vui_info.restricted_ref_pic_lists_flag = reader.readBit();
                vpsInfo.vui_info.min_spatial_segmentation_idc = reader.readUE();
                vpsInfo.vui_info.max_bytes_per_pic_denom = reader.readUE();
                vpsInfo.vui_info.max_bits_per_min_cu_denom = reader.readUE();
                vpsInfo.vui_info.log2_max_mv_length_horizontal = reader.readUE();
                vpsInfo.vui_info.log2_max_mv_length_vertical = reader.readUE();
            }
        }
    } else {
        vpsInfo.vui_parameters_present_flag = 0;
    }

    // 12. vps_extension_flag
    if (reader.hasMoreBits()) {
        vpsInfo.vps_extension_flag = reader.readBit();
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

    if (vpsInfo.vui_parameters_present_flag) {
        std::cout << "\n  VUI Parameters:" << std::endl;

        if (vpsInfo.vui_info.aspect_ratio_info_present_flag) {
            std::cout << "    Aspect Ratio IDC: " << (int)vpsInfo.vui_info.aspect_ratio_idc << std::endl;
            if (vpsInfo.vui_info.aspect_ratio_idc == 255) {
                std::cout << "    SAR Width: " << vpsInfo.vui_info.sar_width << std::endl;
                std::cout << "    SAR Height: " << vpsInfo.vui_info.sar_height << std::endl;
            }
        }

        if (vpsInfo.vui_info.video_signal_type_present_flag) {
            std::cout << "    Video Format: " << (int)vpsInfo.vui_info.video_format << std::endl;
            std::cout << "    Full Range Flag: " << (int)vpsInfo.vui_info.video_full_range_flag << std::endl;

            if (vpsInfo.vui_info.colour_description_present_flag) {
                std::cout << "    Colour Primaries: " << (int)vpsInfo.vui_info.colour_primaries << std::endl;
                std::cout << "    Transfer Characteristics: " << (int)vpsInfo.vui_info.transfer_characteristics << std::endl;
                std::cout << "    Matrix Coefficients: " << (int)vpsInfo.vui_info.matrix_coefficients << std::endl;
            }
        }

        if (vpsInfo.vui_info.chroma_loc_info_present_flag) {
            std::cout << "    Chroma Sample Loc Type Top Field: " << (int)vpsInfo.vui_info.chroma_sample_loc_type_top_field << std::endl;
            std::cout << "    Chroma Sample Loc Type Bottom Field: " << (int)vpsInfo.vui_info.chroma_sample_loc_type_bottom_field << std::endl;
        }

        if (vpsInfo.vui_info.vui_timing_info_present_flag) {
            std::cout << "    Num Units In Tick: " << vpsInfo.vui_info.vui_num_units_in_tick << std::endl;
            std::cout << "    Time Scale: " << vpsInfo.vui_info.vui_time_scale << std::endl;
            std::cout << "    Poc Proportional To Timing Flag: " << (int)vpsInfo.vui_info.vui_poc_proportional_to_timing_flag << std::endl;
        }

        if (vpsInfo.vui_info.bitstream_restriction_flag) {
            std::cout << "    Tiles Fixed Structure Flag: " << (int)vpsInfo.vui_info.tiles_fixed_structure_flag << std::endl;
            std::cout << "    Motion Vectors Over Pic Boundaries Flag: " << (int)vpsInfo.vui_info.motion_vectors_over_pic_boundaries_flag << std::endl;
            std::cout << "    Restricted Ref Pic Lists Flag: " << (int)vpsInfo.vui_info.restricted_ref_pic_lists_flag << std::endl;
        }
    }

    std::cout << std::endl;
}
