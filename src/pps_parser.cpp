#include "pps_parser.h"
#include "bit_stream_reader.h"
#include <iostream>

PPSParser::PPSParser() {
}

PPSParser::~PPSParser() {
}

bool PPSParser::parsePPS(const NALUnit& nalUnit, PPSInfo& ppsInfo) {
    if (nalUnit.nal_unit_type != NAL_UNIT_PPS) {
        return false;
    }

    const uint8_t* data = nalUnit.payload.data();
    size_t size = nalUnit.payload.size();
    BitStreamReader reader(data, size);

    // 1. pps_pic_parameter_set_id — ue(v)
    ppsInfo.pps_pic_parameter_set_id = reader.readUE();

    // 2. pps_seq_parameter_set_id — ue(v)
    ppsInfo.pps_seq_parameter_set_id = reader.readUE();

    // 3. dependent_slice_segments_enabled_flag — u(1)
    ppsInfo.dependent_slice_segments_enabled_flag = reader.readBit();

    // 4. output_flag_present_flag — u(1)
    ppsInfo.output_flag_present_flag = reader.readBit();

    // 5. num_extra_slice_header_bits — u(3) per HEVC spec
    ppsInfo.num_extra_slice_header_bits = reader.readBits(3);

    // 6. num_ref_idx_l0_default_active_minus1 — ue(v)
    ppsInfo.pps_num_ref_idx_l0_default_active_minus1 = reader.readUE();

    // 7. num_ref_idx_l1_default_active_minus1 — ue(v)
    ppsInfo.pps_num_ref_idx_l1_default_active_minus1 = reader.readUE();

    // 8. init_qp_minus26 — se(v) per HEVC spec
    ppsInfo.pps_init_qp_minus26 = reader.readSE();

    // 9. init_qs_minus26 — se(v) (for extended precision, optional in some profiles)
    // In HEVC v1 this is not present; handled by checking remaining bits
    // For compatibility, read as se(v) if bits remain
    if (reader.hasMoreBits()) {
        ppsInfo.pps_init_qs_minus26 = reader.readSE();
    }

    // 10. chroma_qp_index_offset — se(v)
    ppsInfo.pps_chroma_qp_index_offset = reader.readSE();

    // 11. pps_slice_chroma_qp_offsets_present_flag — u(1)
    ppsInfo.pps_slice_chroma_qp_offsets_present_flag = reader.readBit();

    if (ppsInfo.pps_slice_chroma_qp_offsets_present_flag) {
        // 12. pps_cb_qp_offset — se(v)
        ppsInfo.pps_cb_qp_offset = reader.readSE();

        // 13. pps_cr_qp_offset — se(v)
        ppsInfo.pps_cr_qp_offset = reader.readSE();
    }

    // 14. deblocking_filter_control_present_flag — u(1)
    ppsInfo.pps_deblocking_filter_control_present_flag = reader.readBit();

    // 15. constrained_intra_pred_flag — u(1)
    ppsInfo.pps_constrained_intra_pred_flag = reader.readBit();

    // 16. transform_skip_enabled_flag — u(1)
    ppsInfo.pps_transform_skip_enabled_flag = reader.readBit();

    // 17. pps_amvp_enabled_flag — u(1) (for compatibility with earlier codebase)
    ppsInfo.pps_amvp_enabled_flag = reader.readBit();

    // 18. pps_sao_enabled_flag — u(1)
    ppsInfo.pps_sao_enabled_flag = reader.readBit();

    // 19. deblocking filter offsets — se(v) per HEVC spec
    if (ppsInfo.pps_deblocking_filter_control_present_flag) {
        ppsInfo.pps_luma_beta_offset_div2 = reader.readSE();
        ppsInfo.pps_luma_tc_offset_div2 = reader.readSE();
        ppsInfo.pps_chroma_beta_offset_div2 = reader.readSE();
        ppsInfo.pps_chroma_tc_offset_div2 = reader.readSE();
    }

    // 20. num_tile_columns_minus1 — ue(v)
    ppsInfo.pps_num_tile_columns_minus1 = reader.readUE();

    // 21. num_tile_rows_minus1 — ue(v)
    ppsInfo.pps_num_tile_rows_minus1 = reader.readUE();

    // 22. loop_filter_across_tiles_enabled_flag — u(1)
    ppsInfo.pps_loop_filter_across_tiles_enabled_flag = reader.readBit();

    // 23. sample_adaptive_offset_enabled_flag — u(1)
    ppsInfo.pps_sample_adaptive_offset_enabled_flag = reader.readBit();

    // 24. pps_pic_parameter_set_id_extension_flag — u(1)
    if (reader.hasMoreBits()) {
        ppsInfo.pps_pic_parameter_set_id_extension_flag = reader.readBit();
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
    if (ppsInfo.pps_deblocking_filter_control_present_flag) {
        std::cout << "  Luma Beta Offset Div2: " << (int)ppsInfo.pps_luma_beta_offset_div2 << std::endl;
        std::cout << "  Luma Tc Offset Div2: " << (int)ppsInfo.pps_luma_tc_offset_div2 << std::endl;
        std::cout << "  Chroma Beta Offset Div2: " << (int)ppsInfo.pps_chroma_beta_offset_div2 << std::endl;
        std::cout << "  Chroma Tc Offset Div2: " << (int)ppsInfo.pps_chroma_tc_offset_div2 << std::endl;
    }
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
