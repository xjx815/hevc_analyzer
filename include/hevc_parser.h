#ifndef HEVC_PARSER_H
#define HEVC_PARSER_H

#include "hevc_types.h"
#include "nal_unit.h"
#include "sps_parser.h"
#include "pps_parser.h"
#include "vps_parser.h"
#include "frame_analyzer.h"
#include "macroblock_analyzer.h"

class HEVCParser {
public:
    HEVCParser();
    ~HEVCParser();
    
    // 打开并解析 HEVC 码流文件
    bool openFile(const std::string& filePath);
    
    // 解析下一个 NAL 单元
    bool parseNextNALUnit(NALUnit& nalUnit);
    
    // 分析当前 NAL 单元
    void analyzeNALUnit(const NALUnit& nalUnit);
    
    // 关闭文件
    void closeFile();
    
    // 输出码流信息摘要
    void printSummary();
    
private:
    // 文件指针
    FILE* fp;
    
    // 解析器组件
    NALUnitParser nalParser;
    VPSParser vpsParser;
    SPSParser spsParser;
    PPSParser ppsParser;
    FrameAnalyzer frameAnalyzer;
    MacroblockAnalyzer mbAnalyzer;
    
    // 解析状态
    bool fileOpened;
    
    // 存储解析结果
    VPSInfo vpsInfo;
    SPSInfo spsInfo;
    PPSInfo ppsInfo;
    FrameInfo frameInfo;
    std::vector<MacroblockInfo> mbInfos;
    
    // 统计信息
    uint32_t nalUnitCount;
    uint32_t vpsCount;
    uint32_t spsCount;
    uint32_t ppsCount;
    uint32_t frameCount;
    uint32_t mbCount;
};

#endif // HEVC_PARSER_H
