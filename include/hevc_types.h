#ifndef HEVC_TYPES_H
#define HEVC_TYPES_H

#include <stdint.h>
#include <vector>
#include <string>

// NAL 单元类型定义
enum NALUnitType {
    NAL_UNIT_VPS = 32,
    NAL_UNIT_SPS = 33,
    NAL_UNIT_PPS = 34,
    NAL_UNIT_AUD = 35,
    NAL_UNIT_EOS = 36,
    NAL_UNIT_EOB = 37,
    NAL_UNIT_FILLER_DATA = 38,
    NAL_UNIT_SEI_PREFIX = 39,
    NAL_UNIT_SEI_SUFFIX = 40,
    NAL_UNIT_SLICE_TRAIL_N = 0,
    NAL_UNIT_SLICE_TRAIL_R = 1,
    NAL_UNIT_SLICE_TSA_N = 2,
    NAL_UNIT_SLICE_TSA_R = 3,
    NAL_UNIT_SLICE_STSA_N = 4,
    NAL_UNIT_SLICE_STSA_R = 5,
    NAL_UNIT_SLICE_RADL_N = 6,
    NAL_UNIT_SLICE_RADL_R = 7,
    NAL_UNIT_SLICE_RASL_N = 8,
    NAL_UNIT_SLICE_RASL_R = 9,
    NAL_UNIT_SLICE_BLA_W_LP = 16,
    NAL_UNIT_SLICE_BLA_W_RADL = 17,
    NAL_UNIT_SLICE_BLA_N_LP = 18,
    NAL_UNIT_SLICE_IDR_W_RADL = 19,
    NAL_UNIT_SLICE_IDR_N_LP = 20,
    NAL_UNIT_SLICE_CRA = 21,
    NAL_UNIT_SLICE_GDR = 22
};

// VUI 参数结构
typedef struct {
    uint8_t aspect_ratio_info_present_flag;
    uint8_t aspect_ratio_idc;
    uint16_t sar_width;
    uint16_t sar_height;
    uint8_t overscan_info_present_flag;
    uint8_t overscan_appropriate_flag;
    uint8_t video_signal_type_present_flag;
    uint8_t video_format;
    uint8_t video_full_range_flag;
    uint8_t colour_description_present_flag;
    uint8_t colour_primaries;
    uint8_t transfer_characteristics;
    uint8_t matrix_coefficients;
    uint8_t chroma_loc_info_present_flag;
    uint8_t chroma_sample_loc_type_top_field;
    uint8_t chroma_sample_loc_type_bottom_field;
    uint8_t neutral_chroma_indication_flag;
    uint8_t field_seq_flag;
    uint8_t frame_field_info_present_flag;
    uint8_t default_display_window_flag;
    uint32_t def_disp_win_left_offset;
    uint32_t def_disp_win_right_offset;
    uint32_t def_disp_win_top_offset;
    uint32_t def_disp_win_bottom_offset;
    uint8_t vui_timing_info_present_flag;
    uint32_t vui_num_units_in_tick;
    uint32_t vui_time_scale;
    uint8_t vui_poc_proportional_to_timing_flag;
    uint32_t vui_num_ticks_poc_diff_one_minus1;
    uint8_t vui_hrd_parameters_present_flag;
    uint8_t bitstream_restriction_flag;
    uint8_t tiles_fixed_structure_flag;
    uint8_t motion_vectors_over_pic_boundaries_flag;
    uint8_t restricted_ref_pic_lists_flag;
    uint32_t min_spatial_segmentation_idc;
    uint32_t max_bytes_per_pic_denom;
    uint32_t max_bits_per_min_cu_denom;
    uint32_t log2_max_mv_length_horizontal;
    uint32_t log2_max_mv_length_vertical;
} VUIInfo;

// 视频参数集 (VPS) 结构
typedef struct {
    uint8_t vps_video_parameter_set_id;
    uint8_t vps_base_layer_internal_flag;
    uint8_t vps_base_layer_available_flag;
    uint8_t vps_max_layers_minus1;
    uint8_t vps_max_sub_layers_minus1;
    uint8_t vps_temporal_id_nesting_flag;
    uint16_t vps_reserved_0xffff_16bits;
    uint8_t vps_reserved_0x7f_7bits;
    uint32_t vps_max_dec_pic_buffering_minus1[16];
    uint32_t vps_max_num_reorder_pics[16];
    uint32_t vps_max_latency_increase_plus1[16];
    uint8_t vps_reserved_0x1f_5bits;
    uint8_t vps_extension_flag;
    uint8_t vui_parameters_present_flag;
    VUIInfo vui_info;
} VPSInfo;

// 序列参数集 (SPS) 结构
typedef struct {
    uint8_t sps_video_parameter_set_id;
    uint8_t sps_max_sub_layers_minus1;
    uint8_t sps_tier_flag;
    uint8_t sps_profile_idc;
    uint32_t sps_profile_compatibility_flag[32];
    uint8_t sps_progressive_source_flag;
    uint8_t sps_interlaced_source_flag;
    uint8_t sps_non_packed_constraint_flag;
    uint8_t sps_frame_only_constraint_flag;
    uint8_t sps_max_12bit_constraint_flag;
    uint8_t sps_max_10bit_constraint_flag;
    uint8_t sps_max_8bit_constraint_flag;
    uint8_t sps_max_422chroma_constraint_flag;
    uint8_t sps_max_420chroma_constraint_flag;
    uint8_t sps_max_monochrome_constraint_flag;
    uint8_t sps_int_max_wdt_constraint_flag;
    uint8_t sps_int_max_hgt_constraint_flag;
    uint32_t sps_max_dec_pic_buffering_minus1[16];
    uint32_t sps_max_num_reorder_pics[16];
    uint32_t sps_max_latency_increase_plus1[16];
    uint32_t sps_dec_pic_buf_cnt_minus1;
    uint32_t log2_max_pic_order_cnt_lsb_minus4;
    uint8_t sps_sub_layer_ordering_info_present_flag;
    uint32_t sps_max_frame_num_minus4;
    uint8_t sps_num_ref_frames_in_pic_order_cnt_cycle;
    int32_t sps_offset_for_non_ref_pic;
    int32_t sps_offset_for_top_to_bottom_field;
    int32_t sps_offset_for_ref_frame[256];
    uint8_t sps_max_num_ref_frames;
    uint8_t sps_gaps_in_frame_num_value_allowed_flag;
    uint32_t pic_width_in_luma_samples;
    uint32_t pic_height_in_luma_samples;
    uint8_t sps_conformance_window_flag;
    uint32_t conf_win_left_offset;
    uint32_t conf_win_right_offset;
    uint32_t conf_win_top_offset;
    uint32_t conf_win_bottom_offset;
    uint8_t sps_btt_flag;
    uint8_t sps_btp_flag;
    uint8_t sps_dpb_output_delay_handling_flag;
    uint8_t sps_level_idc;
    uint8_t sps_seq_parameter_set_id;
    uint8_t sps_extension_flag;
    uint8_t vui_parameters_present_flag;
    VUIInfo vui_info;
} SPSInfo;

// 图像参数集 (PPS) 结构
typedef struct {
    uint8_t pps_pic_parameter_set_id;
    uint8_t pps_seq_parameter_set_id;
    uint8_t dependent_slice_segments_enabled_flag;
    uint8_t output_flag_present_flag;
    uint8_t num_extra_slice_header_bits;
    uint32_t pps_num_ref_idx_l0_default_active_minus1;
    uint32_t pps_num_ref_idx_l1_default_active_minus1;
    int32_t pps_init_qp_minus26;
    int32_t pps_init_qs_minus26;
    uint8_t pps_chroma_qp_index_offset;
    uint8_t pps_cb_qp_offset;
    uint8_t pps_cr_qp_offset;
    uint8_t pps_slice_chroma_qp_offsets_present_flag;
    uint8_t pps_deblocking_filter_control_present_flag;
    uint8_t pps_constrained_intra_pred_flag;
    uint8_t pps_transform_skip_enabled_flag;
    uint8_t pps_amvp_enabled_flag;
    uint8_t pps_sao_enabled_flag;
    uint8_t pps_luma_beta_offset_div2;
    uint8_t pps_luma_tc_offset_div2;
    uint8_t pps_chroma_beta_offset_div2;
    uint8_t pps_chroma_tc_offset_div2;
    uint8_t pps_num_tile_columns_minus1;
    uint8_t pps_num_tile_rows_minus1;
    uint8_t pps_num_tile_columns_minus1_flag;
    uint8_t pps_num_tile_rows_minus1_flag;
    uint8_t pps_loop_filter_across_tiles_enabled_flag;
    uint8_t pps_pic_width_in_tiles_minus1;
    uint8_t pps_pic_height_in_tiles_minus1;
    uint8_t pps_pic_size_in_map_units_minus1;
    uint8_t pps_sample_adaptive_offset_enabled_flag;
    uint8_t pps_pic_parameter_set_id_extension_flag;
} PPSInfo;

// 帧信息结构
typedef struct {
    uint32_t frame_num;
    uint8_t slice_type;
    uint8_t pic_parameter_set_id;
    uint32_t pic_order_cnt_lsb;
    int32_t delta_poc_bottom;
    int32_t delta_poc[2];
    uint8_t no_output_of_prior_pics_flag;
    uint8_t long_term_reference_flag;
    uint8_t adaptive_ref_pic_marking_mode_flag;
    uint8_t slice_header_extension_present_flag;
} FrameInfo;

// 宏块信息结构
typedef struct {
    uint32_t mb_addr;
    uint8_t mb_type;
    uint8_t pred_mode;
    uint8_t ref_idx_l0;
    uint8_t ref_idx_l1;
    int16_t mv_l0[2];
    int16_t mv_l1[2];
    uint8_t coded_block_pattern;
    uint8_t qp;
} MacroblockInfo;

// NAL 单元结构
typedef struct {
    uint8_t forbidden_zero_bit;
    uint8_t nal_unit_type;
    uint8_t nuh_layer_id;
    uint8_t nuh_temporal_id_plus1;
    std::vector<uint8_t> payload;
} NALUnit;

#endif // HEVC_TYPES_H
