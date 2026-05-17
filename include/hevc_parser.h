#ifndef HEVC_PARSER_H
#define HEVC_PARSER_H

#include "hevc_types.h"
#include "nal_unit.h"
#include "sps_parser.h"
#include "pps_parser.h"
#include "vps_parser.h"
#include "frame_analyzer.h"
#include "macroblock_analyzer.h"
#include <map>

class HEVCParser {
public:
    HEVCParser();
    ~HEVCParser();

    bool openFile(const std::string& filePath);
    bool parseNextNALUnit(NALUnit& nalUnit);
    void analyzeNALUnit(const NALUnit& nalUnit);
    void closeFile();
    void printSummary();

    // For HEIF support: process pre-extracted NAL units
    void analyzeExtractedNALUnit(const NALUnit& nalUnit);

private:
    FILE* fp;

    NALUnitParser nalParser;
    VPSParser vpsParser;
    SPSParser spsParser;
    PPSParser ppsParser;
    FrameAnalyzer frameAnalyzer;
    MacroblockAnalyzer mbAnalyzer;

    bool fileOpened;

    // Store latest parsed parameter sets
    VPSInfo vpsInfo;
    // SPS/PPS maps keyed by ID for slice header cross-referencing
    std::map<uint8_t, SPSInfo> spsMap;
    std::map<uint8_t, PPSInfo> ppsMap;
    SPSInfo latestSPS;
    PPSInfo latestPPS;
    FrameInfo frameInfo;
    std::vector<MacroblockInfo> mbInfos;

    // Statistics
    uint32_t nalUnitCount;
    uint32_t vpsCount;
    uint32_t spsCount;
    uint32_t ppsCount;
    uint32_t frameCount;
    uint32_t mbCount;
    uint32_t seiCount;
    uint32_t audCount;
};

#endif // HEVC_PARSER_H
