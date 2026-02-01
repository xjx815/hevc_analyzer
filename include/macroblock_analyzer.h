#ifndef MACROBLOCK_ANALYZER_H
#define MACROBLOCK_ANALYZER_H

#include "hevc_types.h"

class MacroblockAnalyzer {
public:
    MacroblockAnalyzer();
    ~MacroblockAnalyzer();
    
    // 分析宏块信息
    bool analyzeMacroblock(const NALUnit& nalUnit, const FrameInfo& frameInfo, std::vector<MacroblockInfo>& mbInfos);
    
    // 输出宏块信息
    void printMacroblockInfo(const std::vector<MacroblockInfo>& mbInfos);
    
private:
    // 辅助函数：解析指数哥伦布编码
    uint32_t parseExpGolomb(const uint8_t* data, size_t size, size_t& bitPos);
    
    // 辅助函数：读取指定位置的位
    uint8_t readBit(const uint8_t* data, size_t size, size_t bitPos);
    
    // 辅助函数：读取指定长度的位
    uint32_t readBits(const uint8_t* data, size_t size, size_t& bitPos, uint8_t bitCount);
};

#endif // MACROBLOCK_ANALYZER_H
