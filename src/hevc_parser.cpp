#include "hevc_parser.h"
#include <iostream>
#include <fstream>

HEVCParser::HEVCParser() : fp(nullptr), fileOpened(false),
                           nalUnitCount(0), vpsCount(0), spsCount(0), ppsCount(0),
                           frameCount(0), mbCount(0) {
}

HEVCParser::~HEVCParser() {
    closeFile();
}

bool HEVCParser::openFile(const std::string& filePath) {
    fp = fopen(filePath.c_str(), "rb");
    if (!fp) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return false;
    }
    
    fileOpened = true;
    std::cout << "Successfully opened file: " << filePath << std::endl;
    return true;
}

bool HEVCParser::parseNextNALUnit(NALUnit& nalUnit) {
    if (!fileOpened || !fp) {
        return false;
    }
    
    // 查找 NAL 单元起始码 (0x000001 或 0x00000001)
    uint8_t buffer[4];
    size_t bytesRead = 0;
    
    // 读取数据直到找到起始码
    while (true) {
        // 移动文件指针到当前位置
        if (fread(buffer + bytesRead, 1, 1, fp) != 1) {
            return false; // 文件结束
        }
        
        bytesRead++;
        
        // 检查是否找到起始码
        if (bytesRead >= 3) {
            if ((buffer[bytesRead-3] == 0x00) && (buffer[bytesRead-2] == 0x00) && (buffer[bytesRead-1] == 0x01)) {
                // 找到 0x000001 起始码
                break;
            }
            if (bytesRead >= 4) {
                if ((buffer[bytesRead-4] == 0x00) && (buffer[bytesRead-3] == 0x00) && 
                    (buffer[bytesRead-2] == 0x00) && (buffer[bytesRead-1] == 0x01)) {
                    // 找到 0x00000001 起始码
                    break;
                }
            }
        }
        
        // 如果缓冲区已满，移动数据
        if (bytesRead >= 4) {
            memmove(buffer, buffer + 1, 3);
            bytesRead = 3;
        }
    }
    
    // 读取 NAL 单元数据直到下一个起始码或文件结束
    std::vector<uint8_t> nalData;
    uint8_t byte;
    uint8_t startCodeBuffer[4] = {0};
    size_t startCodePos = 0;
    
    while (fread(&byte, 1, 1, fp) == 1) {
        // 检查是否开始新的起始码
        startCodeBuffer[startCodePos] = byte;
        startCodePos = (startCodePos + 1) % 4;
        
        // 检查是否找到新的起始码
        bool foundNewStartCode = false;
        if ((startCodeBuffer[(startCodePos + 1) % 4] == 0x00) && 
            (startCodeBuffer[(startCodePos + 2) % 4] == 0x00) && 
            (startCodeBuffer[(startCodePos + 3) % 4] == 0x01)) {
            foundNewStartCode = true;
        }
        if ((startCodeBuffer[startCodePos] == 0x01) && 
            (startCodeBuffer[(startCodePos + 1) % 4] == 0x00) && 
            (startCodeBuffer[(startCodePos + 2) % 4] == 0x00) && 
            (startCodeBuffer[(startCodePos + 3) % 4] == 0x00)) {
            foundNewStartCode = true;
        }
        
        if (foundNewStartCode) {
            // 回退文件指针到新起始码的开始
            fseek(fp, -4, SEEK_CUR);
            break;
        }
        
        // 添加字节到 NAL 单元数据
        nalData.push_back(byte);
    }
    
    // 解析 NAL 单元
    if (!nalData.empty()) {
        nalParser.parseNALUnit(nalData.data(), nalData.size(), nalUnit);
        nalUnitCount++;
        return true;
    }
    
    return false;
}

void HEVCParser::analyzeNALUnit(const NALUnit& nalUnit) {
    // 打印 NAL 单元类型（调试信息）
    std::cout << "NAL Unit Type: " << (int)nalUnit.nal_unit_type << std::endl;
    
    // 根据 NAL 单元类型进行不同的分析
    switch (nalUnit.nal_unit_type) {
        case NAL_UNIT_VPS:
            std::cout << "Found VPS unit!" << std::endl;
            if (vpsParser.parseVPS(nalUnit, vpsInfo)) {
                vpsCount++;
                std::cout << "=== VPS Information ===" << std::endl;
                vpsParser.printVPSInfo(vpsInfo);
            }
            break;
        case NAL_UNIT_SPS:
            std::cout << "Found SPS unit!" << std::endl;
            if (spsParser.parseSPS(nalUnit, spsInfo)) {
                spsCount++;
                std::cout << "=== SPS Information ===" << std::endl;
                spsParser.printSPSInfo(spsInfo);
            }
            break;
        case NAL_UNIT_PPS:
            std::cout << "Found PPS unit!" << std::endl;
            if (ppsParser.parsePPS(nalUnit, ppsInfo)) {
                ppsCount++;
                std::cout << "=== PPS Information ===" << std::endl;
                ppsParser.printPPSInfo(ppsInfo);
            }
            break;
        case NAL_UNIT_SLICE_TRAIL_N:
        case NAL_UNIT_SLICE_TRAIL_R:
        case NAL_UNIT_SLICE_TSA_N:
        case NAL_UNIT_SLICE_TSA_R:
        case NAL_UNIT_SLICE_STSA_N:
        case NAL_UNIT_SLICE_STSA_R:
        case NAL_UNIT_SLICE_RADL_N:
        case NAL_UNIT_SLICE_RADL_R:
        case NAL_UNIT_SLICE_RASL_N:
        case NAL_UNIT_SLICE_RASL_R:
        case NAL_UNIT_SLICE_BLA_W_LP:
        case NAL_UNIT_SLICE_BLA_W_RADL:
        case NAL_UNIT_SLICE_BLA_N_LP:
        case NAL_UNIT_SLICE_IDR_W_RADL:
        case NAL_UNIT_SLICE_IDR_N_LP:
        case NAL_UNIT_SLICE_CRA:
        case NAL_UNIT_SLICE_GDR:
            if (frameAnalyzer.analyzeFrame(nalUnit, frameInfo)) {
                frameCount++;
                std::cout << "=== Frame Information ===" << std::endl;
                frameAnalyzer.printFrameInfo(frameInfo);
                
                // 分析宏块信息
                if (mbAnalyzer.analyzeMacroblock(nalUnit, frameInfo, mbInfos)) {
                    mbCount += mbInfos.size();
                    std::cout << "=== Macroblock Information ===" << std::endl;
                    mbAnalyzer.printMacroblockInfo(mbInfos);
                }
            }
            break;
        default:
            // 其他类型的 NAL 单元
            break;
    }
}

void HEVCParser::closeFile() {
    if (fileOpened && fp) {
        fclose(fp);
        fp = nullptr;
        fileOpened = false;
    }
}

void HEVCParser::printSummary() {
    std::cout << "=== HEVC Stream Summary ===" << std::endl;
    std::cout << "Total NAL Units: " << nalUnitCount << std::endl;
    std::cout << "VPS Count: " << vpsCount << std::endl;
    std::cout << "SPS Count: " << spsCount << std::endl;
    std::cout << "PPS Count: " << ppsCount << std::endl;
    std::cout << "Frame Count: " << frameCount << std::endl;
    std::cout << "Total Macroblocks: " << mbCount << std::endl;
    std::cout << std::endl;
}
