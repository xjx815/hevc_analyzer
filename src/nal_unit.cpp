#include "nal_unit.h"
#include <iostream>

NALUnitParser::NALUnitParser() {
}

NALUnitParser::~NALUnitParser() {
}

bool NALUnitParser::parseNALUnitHeader(const uint8_t* data, size_t size, NALUnit& nalUnit) {
    if (size < 2) {
        return false;
    }
    
    // 解析 NAL 单元头
    // 第一个字节：forbidden_zero_bit (1 bit) + nal_unit_type (6 bits)
    nalUnit.forbidden_zero_bit = (data[0] >> 7) & 0x01;
    nalUnit.nal_unit_type = (data[0] >> 1) & 0x3F;
    
    // 第二个字节：nuh_layer_id (6 bits) + nuh_temporal_id_plus1 (3 bits)
    nalUnit.nuh_layer_id = (data[1] >> 3) & 0x3F;
    nalUnit.nuh_temporal_id_plus1 = data[1] & 0x07;
    
    return true;
}

bool NALUnitParser::parseNALUnit(const uint8_t* data, size_t size, NALUnit& nalUnit) {
    if (size < 2) {
        return false;
    }
    
    // 解析 NAL 单元头
    if (!parseNALUnitHeader(data, size, nalUnit)) {
        return false;
    }
    
    // 提取 NAL 单元负载
    size_t payloadSize = size - 2;
    nalUnit.payload.resize(payloadSize);
    memcpy(nalUnit.payload.data(), data + 2, payloadSize);
    
    return true;
}

std::string NALUnitParser::getNALUnitTypeString(uint8_t nalUnitType) {
    switch (nalUnitType) {
        case NAL_UNIT_VPS:
            return "VPS";
        case NAL_UNIT_SPS:
            return "SPS";
        case NAL_UNIT_PPS:
            return "PPS";
        case NAL_UNIT_AUD:
            return "AUD";
        case NAL_UNIT_EOS:
            return "EOS";
        case NAL_UNIT_EOB:
            return "EOB";
        case NAL_UNIT_FILLER_DATA:
            return "FILLER_DATA";
        case NAL_UNIT_SEI_PREFIX:
            return "SEI_PREFIX";
        case NAL_UNIT_SEI_SUFFIX:
            return "SEI_SUFFIX";
        case NAL_UNIT_SLICE_TRAIL_N:
            return "SLICE_TRAIL_N";
        case NAL_UNIT_SLICE_TRAIL_R:
            return "SLICE_TRAIL_R";
        case NAL_UNIT_SLICE_TSA_N:
            return "SLICE_TSA_N";
        case NAL_UNIT_SLICE_TSA_R:
            return "SLICE_TSA_R";
        case NAL_UNIT_SLICE_STSA_N:
            return "SLICE_STSA_N";
        case NAL_UNIT_SLICE_STSA_R:
            return "SLICE_STSA_R";
        case NAL_UNIT_SLICE_RADL_N:
            return "SLICE_RADL_N";
        case NAL_UNIT_SLICE_RADL_R:
            return "SLICE_RADL_R";
        case NAL_UNIT_SLICE_RASL_N:
            return "SLICE_RASL_N";
        case NAL_UNIT_SLICE_RASL_R:
            return "SLICE_RASL_R";
        case NAL_UNIT_SLICE_BLA_W_LP:
            return "SLICE_BLA_W_LP";
        case NAL_UNIT_SLICE_BLA_W_RADL:
            return "SLICE_BLA_W_RADL";
        case NAL_UNIT_SLICE_BLA_N_LP:
            return "SLICE_BLA_N_LP";
        case NAL_UNIT_SLICE_IDR_W_RADL:
            return "SLICE_IDR_W_RADL";
        case NAL_UNIT_SLICE_IDR_N_LP:
            return "SLICE_IDR_N_LP";
        case NAL_UNIT_SLICE_CRA:
            return "SLICE_CRA";
        case NAL_UNIT_SLICE_GDR:
            return "SLICE_GDR";
        default:
            return "UNKNOWN";
    }
}

void NALUnitParser::printNALUnitInfo(const NALUnit& nalUnit) {
    std::cout << "NAL Unit Info:" << std::endl;
    std::cout << "  Type: " << getNALUnitTypeString(nalUnit.nal_unit_type) << " (" << (int)nalUnit.nal_unit_type << ")" << std::endl;
    std::cout << "  Forbidden Zero Bit: " << (int)nalUnit.forbidden_zero_bit << std::endl;
    std::cout << "  Layer ID: " << (int)nalUnit.nuh_layer_id << std::endl;
    std::cout << "  Temporal ID: " << (int)(nalUnit.nuh_temporal_id_plus1 - 1) << std::endl;
    std::cout << "  Payload Size: " << nalUnit.payload.size() << " bytes" << std::endl;
    std::cout << std::endl;
}
