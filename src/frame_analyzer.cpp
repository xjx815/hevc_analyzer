#include "frame_analyzer.h"
#include "bit_stream_reader.h"
#include <iostream>

FrameAnalyzer::FrameAnalyzer() {
}

FrameAnalyzer::~FrameAnalyzer() {
}

bool FrameAnalyzer::analyzeFrame(const NALUnit& nalUnit, FrameInfo& frameInfo,
                                  const SPSInfo* spsInfo) {
    if (nalUnit.nal_unit_type < NAL_UNIT_SLICE_TRAIL_N || nalUnit.nal_unit_type > NAL_UNIT_SLICE_GDR) {
        return false;
    }

    const uint8_t* data = nalUnit.payload.data();
    size_t size = nalUnit.payload.size();
    BitStreamReader reader(data, size);

    // 1. first_slice_segment_in_pic_flag — u(1)
    uint8_t firstSliceSegmentInPicFlag = reader.readBit();

    // 2. slice_type — ue(v)
    frameInfo.slice_type = reader.readUE();

    // 3. pic_parameter_set_id — ue(v)
    frameInfo.pic_parameter_set_id = reader.readUE();

    // 4. frame_num — use SPS-derived bit width or default
    uint8_t log2MaxFrameNum;
    if (spsInfo && spsInfo->sps_max_frame_num_minus4 > 0) {
        log2MaxFrameNum = spsInfo->sps_max_frame_num_minus4 + 4;
    } else {
        log2MaxFrameNum = 8; // default fallback
    }
    frameInfo.frame_num = reader.readBits(log2MaxFrameNum);

    if (firstSliceSegmentInPicFlag) {
        // 5. pic_order_cnt_lsb — use SPS-derived bit width or default
        uint8_t log2MaxPicOrderCntLsb;
        if (spsInfo && spsInfo->log2_max_pic_order_cnt_lsb_minus4 > 0) {
            log2MaxPicOrderCntLsb = spsInfo->log2_max_pic_order_cnt_lsb_minus4 + 4;
        } else {
            log2MaxPicOrderCntLsb = 8; // default fallback
        }
        frameInfo.pic_order_cnt_lsb = reader.readBits(log2MaxPicOrderCntLsb);

        // 6. delta_poc_bottom — se(v) when applicable
        // Note: only present for specific slice types; simplified here
        frameInfo.delta_poc_bottom = reader.readSE();

        // 7. no_output_of_prior_pics_flag — u(1)
        frameInfo.no_output_of_prior_pics_flag = reader.readBit();

        // 8. long_term_reference_flag — u(1)
        frameInfo.long_term_reference_flag = reader.readBit();
    }

    // 9. adaptive_ref_pic_marking_mode_flag — u(1)
    frameInfo.adaptive_ref_pic_marking_mode_flag = reader.readBit();

    // 10. slice_header_extension_present_flag — u(1)
    frameInfo.slice_header_extension_present_flag = reader.readBit();

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
