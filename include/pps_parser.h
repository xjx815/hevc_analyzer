#ifndef PPS_PARSER_H
#define PPS_PARSER_H

#include "hevc_types.h"

class PPSParser {
public:
    PPSParser();
    ~PPSParser();

    bool parsePPS(const NALUnit& nalUnit, PPSInfo& ppsInfo);
    void printPPSInfo(const PPSInfo& ppsInfo);
};

#endif // PPS_PARSER_H
