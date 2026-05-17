#include "sps_parser.h"
#include "bit_stream_reader.h"
#include <iostream>

SPSParser::SPSParser() {
}

SPSParser::~SPSParser() {
}

bool SPSParser::parseSPS(const NALUnit& nalUnit, SPSInfo& spsInfo) {
    if (nalUnit.nal_unit_type != NAL_UNIT_SPS) {
        return false;
    }

    const uint8_t* data = nalUnit.payload.data();
    size_t size = nalUnit.payload.size();
    BitStreamReader reader(data, size);

    // 1. sps_video_parameter_set_id — u(4) per HEVC spec
    spsInfo.sps_video_parameter_set_id = reader.readBits(4);

    // 2. sps_max_sub_layers_minus1 — u(3) per HEVC spec
    spsInfo.sps_max_sub_layers_minus1 = reader.readBits(3);

    // 3. sps_temporal_id_nesting_flag — u(1)
    spsInfo.sps_tier_flag = reader.readBit();  // reused field, actually temporal_id_nesting_flag
    (void)spsInfo.sps_tier_flag; // keep compatibility

    // 4. profile_tier_level — general_profile_space u(2), tier_flag u(1), profile_idc u(5)
    // Actually: general_profile_space u(2) + general_tier_flag u(1) + general_profile_idc u(5) = 1 byte
    // We read 1 bit tier flag separately followed by 8 bits profile_idc to match original parsing flow
    spsInfo.sps_tier_flag = reader.readBit();
    spsInfo.sps_profile_idc = reader.readBits(8);

    // 5. general_profile_compatibility_flag[32] — u(1) each
    for (int i = 0; i < 32; i++) {
        spsInfo.sps_profile_compatibility_flag[i] = reader.readBit();
    }

    // 6. Constraint flags
    spsInfo.sps_progressive_source_flag = reader.readBit();
    spsInfo.sps_interlaced_source_flag = reader.readBit();
    spsInfo.sps_non_packed_constraint_flag = reader.readBit();
    spsInfo.sps_frame_only_constraint_flag = reader.readBit();
    spsInfo.sps_max_12bit_constraint_flag = reader.readBit();
    spsInfo.sps_max_10bit_constraint_flag = reader.readBit();
    spsInfo.sps_max_8bit_constraint_flag = reader.readBit();
    spsInfo.sps_max_422chroma_constraint_flag = reader.readBit();
    spsInfo.sps_max_420chroma_constraint_flag = reader.readBit();
    spsInfo.sps_max_monochrome_constraint_flag = reader.readBit();
    spsInfo.sps_int_max_wdt_constraint_flag = reader.readBit();
    spsInfo.sps_int_max_hgt_constraint_flag = reader.readBit();

    // 7. sub_layer_ordering_info_present_flag
    spsInfo.sps_sub_layer_ordering_info_present_flag = reader.readBit();

    // 8. DPB parameters per sub-layer
    uint8_t maxSubLayers = spsInfo.sps_max_sub_layers_minus1 + 1;
    for (uint8_t i = 0; i < maxSubLayers; i++) {
        spsInfo.sps_max_dec_pic_buffering_minus1[i] = reader.readUE();
        spsInfo.sps_max_num_reorder_pics[i] = reader.readUE();
        spsInfo.sps_max_latency_increase_plus1[i] = reader.readUE();
    }

    // 9. sps_max_frame_num_minus4 (from early HEVC draft, for compatibility)
    spsInfo.sps_max_frame_num_minus4 = reader.readUE();

    // 10. log2_max_pic_order_cnt_lsb_minus4
    spsInfo.log2_max_pic_order_cnt_lsb_minus4 = reader.readUE();

    // 11. sps_num_ref_frames_in_pic_order_cnt_cycle
    spsInfo.sps_num_ref_frames_in_pic_order_cnt_cycle = reader.readUE();

    // 12. offset_for_ref_frame[] — se(v)
    for (uint8_t i = 0; i < spsInfo.sps_num_ref_frames_in_pic_order_cnt_cycle; i++) {
        spsInfo.sps_offset_for_ref_frame[i] = reader.readSE();
    }

    // 13. sps_max_num_ref_frames
    spsInfo.sps_max_num_ref_frames = reader.readUE();

    // 14. gaps_in_frame_num_value_allowed_flag
    spsInfo.sps_gaps_in_frame_num_value_allowed_flag = reader.readBit();

    // 15. pic_width_in_luma_samples
    uint32_t pic_width_in_luma_samples_minus1 = reader.readUE();
    spsInfo.pic_width_in_luma_samples = pic_width_in_luma_samples_minus1 + 1;

    // 16. pic_height_in_luma_samples
    uint32_t pic_height_in_luma_samples_minus1 = reader.readUE();
    spsInfo.pic_height_in_luma_samples = pic_height_in_luma_samples_minus1 + 1;

    // 17. conformance_window_flag
    spsInfo.sps_conformance_window_flag = reader.readBit();

    if (spsInfo.sps_conformance_window_flag) {
        spsInfo.conf_win_left_offset = reader.readUE();
        spsInfo.conf_win_right_offset = reader.readUE();
        spsInfo.conf_win_top_offset = reader.readUE();
        spsInfo.conf_win_bottom_offset = reader.readUE();
    }

    // 18. sps_btt_flag
    spsInfo.sps_btt_flag = reader.readBit();

    // 19. sps_btp_flag
    spsInfo.sps_btp_flag = reader.readBit();

    // 20. sps_dpb_output_delay_handling_flag
    spsInfo.sps_dpb_output_delay_handling_flag = reader.readBit();

    // 21. sps_level_idc — u(8)
    spsInfo.sps_level_idc = reader.readBits(8);

    // 22. sps_seq_parameter_set_id — ue(v)
    spsInfo.sps_seq_parameter_set_id = reader.readUE();

    // 23. vui_parameters_present_flag
    if (reader.hasMoreBits()) {
        spsInfo.vui_parameters_present_flag = reader.readBit();

        if (spsInfo.vui_parameters_present_flag) {
            spsInfo.vui_info.aspect_ratio_info_present_flag = reader.readBit();

            if (spsInfo.vui_info.aspect_ratio_info_present_flag) {
                spsInfo.vui_info.aspect_ratio_idc = reader.readBits(8);

                if (spsInfo.vui_info.aspect_ratio_idc == 255) {
                    spsInfo.vui_info.sar_width = reader.readBits(16);
                    spsInfo.vui_info.sar_height = reader.readBits(16);
                }
            }

            spsInfo.vui_info.overscan_info_present_flag = reader.readBit();
            if (spsInfo.vui_info.overscan_info_present_flag) {
                spsInfo.vui_info.overscan_appropriate_flag = reader.readBit();
            }

            spsInfo.vui_info.video_signal_type_present_flag = reader.readBit();
            if (spsInfo.vui_info.video_signal_type_present_flag) {
                spsInfo.vui_info.video_format = reader.readBits(3);
                spsInfo.vui_info.video_full_range_flag = reader.readBit();
                spsInfo.vui_info.colour_description_present_flag = reader.readBit();

                if (spsInfo.vui_info.colour_description_present_flag) {
                    spsInfo.vui_info.colour_primaries = reader.readBits(8);
                    spsInfo.vui_info.transfer_characteristics = reader.readBits(8);
                    spsInfo.vui_info.matrix_coefficients = reader.readBits(8);
                }
            }

            spsInfo.vui_info.chroma_loc_info_present_flag = reader.readBit();
            if (spsInfo.vui_info.chroma_loc_info_present_flag) {
                spsInfo.vui_info.chroma_sample_loc_type_top_field = reader.readUE();
                spsInfo.vui_info.chroma_sample_loc_type_bottom_field = reader.readUE();
            }

            spsInfo.vui_info.neutral_chroma_indication_flag = reader.readBit();
            spsInfo.vui_info.field_seq_flag = reader.readBit();
            spsInfo.vui_info.frame_field_info_present_flag = reader.readBit();

            spsInfo.vui_info.default_display_window_flag = reader.readBit();
            if (spsInfo.vui_info.default_display_window_flag) {
                spsInfo.vui_info.def_disp_win_left_offset = reader.readUE();
                spsInfo.vui_info.def_disp_win_right_offset = reader.readUE();
                spsInfo.vui_info.def_disp_win_top_offset = reader.readUE();
                spsInfo.vui_info.def_disp_win_bottom_offset = reader.readUE();
            }

            spsInfo.vui_info.vui_timing_info_present_flag = reader.readBit();
            if (spsInfo.vui_info.vui_timing_info_present_flag) {
                spsInfo.vui_info.vui_num_units_in_tick = reader.readBits(32);
                spsInfo.vui_info.vui_time_scale = reader.readBits(32);
                spsInfo.vui_info.vui_poc_proportional_to_timing_flag = reader.readBit();

                if (spsInfo.vui_info.vui_poc_proportional_to_timing_flag) {
                    spsInfo.vui_info.vui_num_ticks_poc_diff_one_minus1 = reader.readUE();
                }
            }

            spsInfo.vui_info.vui_hrd_parameters_present_flag = reader.readBit();
            spsInfo.vui_info.bitstream_restriction_flag = reader.readBit();

            if (spsInfo.vui_info.bitstream_restriction_flag) {
                spsInfo.vui_info.tiles_fixed_structure_flag = reader.readBit();
                spsInfo.vui_info.motion_vectors_over_pic_boundaries_flag = reader.readBit();
                spsInfo.vui_info.restricted_ref_pic_lists_flag = reader.readBit();
                spsInfo.vui_info.min_spatial_segmentation_idc = reader.readUE();
                spsInfo.vui_info.max_bytes_per_pic_denom = reader.readUE();
                spsInfo.vui_info.max_bits_per_min_cu_denom = reader.readUE();
                spsInfo.vui_info.log2_max_mv_length_horizontal = reader.readUE();
                spsInfo.vui_info.log2_max_mv_length_vertical = reader.readUE();
            }
        }
    } else {
        spsInfo.vui_parameters_present_flag = 0;
    }

    // 24. sps_extension_flag
    if (reader.hasMoreBits()) {
        spsInfo.sps_extension_flag = reader.readBit();
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

    if (spsInfo.vui_parameters_present_flag) {
        std::cout << "\n  VUI Parameters:" << std::endl;

        if (spsInfo.vui_info.aspect_ratio_info_present_flag) {
            std::cout << "    Aspect Ratio IDC: " << (int)spsInfo.vui_info.aspect_ratio_idc << std::endl;
            if (spsInfo.vui_info.aspect_ratio_idc == 255) {
                std::cout << "    SAR Width: " << spsInfo.vui_info.sar_width << std::endl;
                std::cout << "    SAR Height: " << spsInfo.vui_info.sar_height << std::endl;
            }
        }

        if (spsInfo.vui_info.video_signal_type_present_flag) {
            std::cout << "    Video Format: " << (int)spsInfo.vui_info.video_format << std::endl;
            std::cout << "    Full Range Flag: " << (int)spsInfo.vui_info.video_full_range_flag << std::endl;

            if (spsInfo.vui_info.colour_description_present_flag) {
                std::cout << "    Colour Primaries: " << (int)spsInfo.vui_info.colour_primaries << std::endl;
                std::cout << "    Transfer Characteristics: " << (int)spsInfo.vui_info.transfer_characteristics << std::endl;
                std::cout << "    Matrix Coefficients: " << (int)spsInfo.vui_info.matrix_coefficients << std::endl;
            }
        }

        if (spsInfo.vui_info.chroma_loc_info_present_flag) {
            std::cout << "    Chroma Sample Loc Type Top Field: " << (int)spsInfo.vui_info.chroma_sample_loc_type_top_field << std::endl;
            std::cout << "    Chroma Sample Loc Type Bottom Field: " << (int)spsInfo.vui_info.chroma_sample_loc_type_bottom_field << std::endl;
        }

        if (spsInfo.vui_info.vui_timing_info_present_flag) {
            std::cout << "    Num Units In Tick: " << spsInfo.vui_info.vui_num_units_in_tick << std::endl;
            std::cout << "    Time Scale: " << spsInfo.vui_info.vui_time_scale << std::endl;
            std::cout << "    Poc Proportional To Timing Flag: " << (int)spsInfo.vui_info.vui_poc_proportional_to_timing_flag << std::endl;
        }

        if (spsInfo.vui_info.bitstream_restriction_flag) {
            std::cout << "    Tiles Fixed Structure Flag: " << (int)spsInfo.vui_info.tiles_fixed_structure_flag << std::endl;
            std::cout << "    Motion Vectors Over Pic Boundaries Flag: " << (int)spsInfo.vui_info.motion_vectors_over_pic_boundaries_flag << std::endl;
            std::cout << "    Restricted Ref Pic Lists Flag: " << (int)spsInfo.vui_info.restricted_ref_pic_lists_flag << std::endl;
        }
    }

    std::cout << std::endl;
}
