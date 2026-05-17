#include "hevc_parser.h"
#include <iostream>
#include <fstream>

HEVCParser::HEVCParser() : fp(nullptr), fileOpened(false),
                           nalUnitCount(0), vpsCount(0), spsCount(0), ppsCount(0),
                           frameCount(0), mbCount(0), seiCount(0), audCount(0) {
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

    uint8_t buffer[4];
    size_t bytesRead = 0;

    // Find NAL unit start code (0x000001 or 0x00000001)
    while (true) {
        if (fread(buffer + bytesRead, 1, 1, fp) != 1) {
            return false; // EOF
        }

        bytesRead++;

        if (bytesRead >= 3) {
            if ((buffer[bytesRead-3] == 0x00) && (buffer[bytesRead-2] == 0x00) && (buffer[bytesRead-1] == 0x01)) {
                break;
            }
            if (bytesRead >= 4) {
                if ((buffer[bytesRead-4] == 0x00) && (buffer[bytesRead-3] == 0x00) &&
                    (buffer[bytesRead-2] == 0x00) && (buffer[bytesRead-1] == 0x01)) {
                    break;
                }
            }
        }

        if (bytesRead >= 4) {
            memmove(buffer, buffer + 1, 3);
            bytesRead = 3;
        }
    }

    // Read NAL unit data until next start code or EOF
    std::vector<uint8_t> nalData;
    uint8_t byte;
    uint8_t startCodeBuffer[4] = {0};
    size_t startCodePos = 0;

    while (fread(&byte, 1, 1, fp) == 1) {
        startCodeBuffer[startCodePos] = byte;
        startCodePos = (startCodePos + 1) % 4;

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
            fseek(fp, -4, SEEK_CUR);
            break;
        }

        nalData.push_back(byte);
    }

    if (!nalData.empty()) {
        nalParser.parseNALUnit(nalData.data(), nalData.size(), nalUnit);
        nalUnitCount++;
        return true;
    }

    return false;
}

void HEVCParser::analyzeNALUnit(const NALUnit& nalUnit) {
    analyzeExtractedNALUnit(nalUnit);
}

void HEVCParser::analyzeExtractedNALUnit(const NALUnit& nalUnit) {
    std::cout << "NAL Unit Type: " << (int)nalUnit.nal_unit_type << std::endl;

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
            {
                SPSInfo spsInfo;
                if (spsParser.parseSPS(nalUnit, spsInfo)) {
                    spsCount++;
                    latestSPS = spsInfo;
                    spsMap[spsInfo.sps_seq_parameter_set_id] = spsInfo;
                    std::cout << "=== SPS Information ===" << std::endl;
                    spsParser.printSPSInfo(spsInfo);
                }
            }
            break;
        case NAL_UNIT_PPS:
            std::cout << "Found PPS unit!" << std::endl;
            {
                PPSInfo ppsInfo;
                if (ppsParser.parsePPS(nalUnit, ppsInfo)) {
                    ppsCount++;
                    latestPPS = ppsInfo;
                    ppsMap[ppsInfo.pps_pic_parameter_set_id] = ppsInfo;
                    std::cout << "=== PPS Information ===" << std::endl;
                    ppsParser.printPPSInfo(ppsInfo);
                }
            }
            break;
        case NAL_UNIT_AUD:
            audCount++;
            std::cout << "Found AUD unit!" << std::endl;
            break;
        case NAL_UNIT_EOS:
            std::cout << "Found EOS unit!" << std::endl;
            break;
        case NAL_UNIT_EOB:
            std::cout << "Found EOB unit!" << std::endl;
            break;
        case NAL_UNIT_FILLER_DATA:
            std::cout << "Found FILLER DATA unit!" << std::endl;
            break;
        case NAL_UNIT_SEI_PREFIX:
        case NAL_UNIT_SEI_SUFFIX:
            seiCount++;
            std::cout << "Found SEI unit (" << (nalUnit.nal_unit_type == NAL_UNIT_SEI_PREFIX ? "PREFIX" : "SUFFIX") << ")" << std::endl;
            {
                // Basic SEI message parsing
                const uint8_t* data = nalUnit.payload.data();
                size_t size = nalUnit.payload.size();
                size_t bytePos = 0;
                int msgNum = 0;
                while (bytePos < size) {
                    // Parse payload type (0xFF continuation)
                    uint32_t payloadType = 0;
                    while (bytePos < size && data[bytePos] == 0xFF) {
                        payloadType += 255;
                        bytePos++;
                    }
                    if (bytePos < size) {
                        payloadType += data[bytePos];
                        bytePos++;
                    } else break;

                    // Parse payload size (0xFF continuation)
                    uint32_t payloadSize = 0;
                    while (bytePos < size && data[bytePos] == 0xFF) {
                        payloadSize += 255;
                        bytePos++;
                    }
                    if (bytePos < size) {
                        payloadSize += data[bytePos];
                        bytePos++;
                    } else break;

                    std::cout << "  SEI message #" << msgNum
                              << ": type=" << payloadType
                              << ", size=" << payloadSize << " bytes" << std::endl;
                    bytePos += payloadSize;
                    msgNum++;
                }
                std::cout << "  Total SEI messages: " << msgNum << std::endl;
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
            {
                // Look up SPS by current PPS's sps_id if available
                const SPSInfo* spsPtr = nullptr;
                if (!ppsMap.empty()) {
                    // Use the most recent PPS to find its SPS
                    auto ppsIt = ppsMap.find(latestPPS.pps_pic_parameter_set_id);
                    if (ppsIt != ppsMap.end()) {
                        auto spsIt = spsMap.find(ppsIt->second.pps_seq_parameter_set_id);
                        if (spsIt != spsMap.end()) {
                            spsPtr = &spsIt->second;
                        }
                    }
                }
                if (!spsPtr && !spsMap.empty()) {
                    spsPtr = &latestSPS;
                }

                if (frameAnalyzer.analyzeFrame(nalUnit, frameInfo, spsPtr)) {
                    frameCount++;
                    std::cout << "=== Frame Information ===" << std::endl;
                    frameAnalyzer.printFrameInfo(frameInfo);

                    if (mbAnalyzer.analyzeMacroblock(nalUnit, frameInfo, mbInfos, spsPtr)) {
                        mbCount += mbInfos.size();
                        std::cout << "=== Macroblock Information ===" << std::endl;
                        mbAnalyzer.printMacroblockInfo(mbInfos);
                    }
                }
            }
            break;
        default:
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
    std::cout << "AUD Count: " << audCount << std::endl;
    std::cout << "SEI Count: " << seiCount << std::endl;
    std::cout << "Frame Count: " << frameCount << std::endl;
    std::cout << "Total Macroblocks: " << mbCount << std::endl;
    std::cout << std::endl;
}
