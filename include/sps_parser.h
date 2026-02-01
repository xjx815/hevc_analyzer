#ifndef SPS_PARSER_H
#define SPS_PARSER_H

#include "hevc_types.h"

class SPSParser {
public:
    SPSParser();
    ~SPSParser();
    
    // 解析 SPS 信息
    bool parseSPS(const NALUnit& nalUnit, SPSInfo& spsInfo);
    
    // 输出 SPS 信息
    void printSPSInfo(const SPSInfo& spsInfo);
    
private:
    // 辅助函数：解析指数哥伦布编码
    uint32_t parseExpGolomb(const uint8_t* data, size_t size, size_t& bitPos);
    
    // 辅助函数：读取指定位置的位
    uint8_t readBit(const uint8_t* data, size_t size, size_t bitPos);
    
    // 辅助函数：读取指定长度的位
    uint32_t readBits(const uint8_t* data, size_t size, size_t& bitPos, uint8_t bitCount);
};

#endif // SPS_PARSER_H
