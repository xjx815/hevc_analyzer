#ifndef VPS_PARSER_H
#define VPS_PARSER_H

#include "hevc_types.h"

class VPSParser {
public:
    VPSParser();
    ~VPSParser();
    
    bool parseVPS(const NALUnit& nalUnit, VPSInfo& vpsInfo);
    void printVPSInfo(const VPSInfo& vpsInfo);
    
private:
    uint8_t readBit(const uint8_t* data, size_t size, size_t bitPos);
    uint32_t readBits(const uint8_t* data, size_t size, size_t& bitPos, uint8_t bitCount);
    uint32_t parseExpGolomb(const uint8_t* data, size_t size, size_t& bitPos);
};

#endif // VPS_PARSER_H
