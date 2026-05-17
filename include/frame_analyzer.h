#ifndef FRAME_ANALYZER_H
#define FRAME_ANALYZER_H

#include "hevc_types.h"

class FrameAnalyzer {
public:
    FrameAnalyzer();
    ~FrameAnalyzer();

    bool analyzeFrame(const NALUnit& nalUnit, FrameInfo& frameInfo,
                      const SPSInfo* spsInfo = nullptr);
    void printFrameInfo(const FrameInfo& frameInfo);
};

#endif // FRAME_ANALYZER_H
