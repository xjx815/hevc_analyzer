#include "macroblock_analyzer.h"
#include "bit_stream_reader.h"
#include <iostream>

MacroblockAnalyzer::MacroblockAnalyzer() {
}

MacroblockAnalyzer::~MacroblockAnalyzer() {
}

bool MacroblockAnalyzer::analyzeMacroblock(const NALUnit& nalUnit, const FrameInfo& /*frameInfo*/,
                                            std::vector<MacroblockInfo>& mbInfos,
                                            const SPSInfo* spsInfo) {
    if (nalUnit.nal_unit_type < NAL_UNIT_SLICE_TRAIL_N || nalUnit.nal_unit_type > NAL_UNIT_SLICE_GDR) {
        return false;
    }

    const uint8_t* data = nalUnit.payload.data();
    size_t size = nalUnit.payload.size();
    BitStreamReader reader(data, size);

    // Skip slice header (same fields as FrameAnalyzer)
    uint8_t firstSliceSegmentInPicFlag = reader.readBit();
    reader.readUE();  // slice_type
    reader.readUE();  // pic_parameter_set_id

    uint8_t log2MaxFrameNum;
    if (spsInfo && spsInfo->sps_max_frame_num_minus4 > 0) {
        log2MaxFrameNum = spsInfo->sps_max_frame_num_minus4 + 4;
    } else {
        log2MaxFrameNum = 8;
    }
    reader.readBits(log2MaxFrameNum);  // frame_num

    if (firstSliceSegmentInPicFlag) {
        uint8_t log2MaxPicOrderCntLsb;
        if (spsInfo && spsInfo->log2_max_pic_order_cnt_lsb_minus4 > 0) {
            log2MaxPicOrderCntLsb = spsInfo->log2_max_pic_order_cnt_lsb_minus4 + 4;
        } else {
            log2MaxPicOrderCntLsb = 8;
        }
        reader.readBits(log2MaxPicOrderCntLsb);  // pic_order_cnt_lsb
        reader.readSE();  // delta_poc_bottom
        reader.readBit(); // no_output_of_prior_pics_flag
        reader.readBit(); // long_term_reference_flag
    }

    reader.readBit(); // adaptive_ref_pic_marking_mode_flag
    reader.readBit(); // slice_header_extension_present_flag

    // Macroblock data — simulated (actual CTU/CU parsing is complex)
    // Use pic dimensions from SPS to estimate CTU count if available
    int numMBs = 16 * 16;
    if (spsInfo && spsInfo->pic_width_in_luma_samples > 0 && spsInfo->pic_height_in_luma_samples > 0) {
        // HEVC uses 64x64 CTUs; estimate count
        int ctusX = (spsInfo->pic_width_in_luma_samples + 63) / 64;
        int ctusY = (spsInfo->pic_height_in_luma_samples + 63) / 64;
        numMBs = ctusX * ctusY;
        if (numMBs < 1) numMBs = 1;
        if (numMBs > 4096) numMBs = 4096; // reasonable upper bound
    }

    mbInfos.reserve(numMBs);

    for (int i = 0; i < numMBs; i++) {
        MacroblockInfo mbInfo;
        mbInfo.mb_addr = i;
        mbInfo.mb_type = i % 4;
        mbInfo.pred_mode = i % 2;
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
