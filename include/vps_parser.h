#ifndef VPS_PARSER_H
#define VPS_PARSER_H

#include "hevc_types.h"

class VPSParser {
public:
    VPSParser();
    ~VPSParser();

    bool parseVPS(const NALUnit& nalUnit, VPSInfo& vpsInfo);
    void printVPSInfo(const VPSInfo& vpsInfo);
};

#endif // VPS_PARSER_H
