#ifndef MACROBLOCK_ANALYZER_H
#define MACROBLOCK_ANALYZER_H

#include "hevc_types.h"

class MacroblockAnalyzer {
public:
    MacroblockAnalyzer();
    ~MacroblockAnalyzer();

    bool analyzeMacroblock(const NALUnit& nalUnit, const FrameInfo& frameInfo,
                           std::vector<MacroblockInfo>& mbInfos,
                           const SPSInfo* spsInfo = nullptr);
    void printMacroblockInfo(const std::vector<MacroblockInfo>& mbInfos);
};

#endif // MACROBLOCK_ANALYZER_H
