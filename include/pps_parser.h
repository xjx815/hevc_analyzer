#ifndef PPS_PARSER_H
#define PPS_PARSER_H

#include "hevc_types.h"

class PPSParser {
public:
    PPSParser();
    ~PPSParser();
    
    // 解析 PPS 信息
    bool parsePPS(const NALUnit& nalUnit, PPSInfo& ppsInfo);
    
    // 输出 PPS 信息
    void printPPSInfo(const PPSInfo& ppsInfo);
    
private:
    // 辅助函数：解析指数哥伦布编码
    uint32_t parseExpGolomb(const uint8_t* data, size_t size, size_t& bitPos);
    
    // 辅助函数：读取指定位置的位
    uint8_t readBit(const uint8_t* data, size_t size, size_t bitPos);
    
    // 辅助函数：读取指定长度的位
    uint32_t readBits(const uint8_t* data, size_t size, size_t& bitPos, uint8_t bitCount);
};

#endif // PPS_PARSER_H
