#ifndef FRAME_ANALYZER_H
#define FRAME_ANALYZER_H

#include "hevc_types.h"

class FrameAnalyzer {
public:
    FrameAnalyzer();
    ~FrameAnalyzer();
    
    // 分析帧信息
    bool analyzeFrame(const NALUnit& nalUnit, FrameInfo& frameInfo);
    
    // 输出帧信息
    void printFrameInfo(const FrameInfo& frameInfo);
    
private:
    // 辅助函数：解析指数哥伦布编码
    uint32_t parseExpGolomb(const uint8_t* data, size_t size, size_t& bitPos);
    
    // 辅助函数：读取指定位置的位
    uint8_t readBit(const uint8_t* data, size_t size, size_t bitPos);
    
    // 辅助函数：读取指定长度的位
    uint32_t readBits(const uint8_t* data, size_t size, size_t& bitPos, uint8_t bitCount);
};

#endif // FRAME_ANALYZER_H
