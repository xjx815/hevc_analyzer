#include "hevc_parser.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: hevc_analyzer <hevc_file>" << std::endl;
        return 1;
    }
    
    std::string filePath = argv[1];
    HEVCParser parser;
    
    // 打开文件
    if (!parser.openFile(filePath)) {
        return 1;
    }
    
    // 解析码流
    NALUnit nalUnit;
    while (parser.parseNextNALUnit(nalUnit)) {
        // 分析 NAL 单元
        parser.analyzeNALUnit(nalUnit);
    }
    
    // 输出摘要
    parser.printSummary();
    
    // 关闭文件
    parser.closeFile();
    
    return 0;
}
