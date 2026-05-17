#ifndef SPS_PARSER_H
#define SPS_PARSER_H

#include "hevc_types.h"

class SPSParser {
public:
    SPSParser();
    ~SPSParser();

    bool parseSPS(const NALUnit& nalUnit, SPSInfo& spsInfo);
    void printSPSInfo(const SPSInfo& spsInfo);
};

#endif // SPS_PARSER_H
