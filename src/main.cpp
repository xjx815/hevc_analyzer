#include "hevc_parser.h"
#include "heif_parser.h"
#include <iostream>
#include <string>
#include <cstring>

static void processRawHEVC(const std::string& filePath) {
    HEVCParser parser;

    if (!parser.openFile(filePath)) {
        return;
    }

    NALUnit nalUnit;
    while (parser.parseNextNALUnit(nalUnit)) {
        parser.analyzeNALUnit(nalUnit);
    }

    parser.printSummary();
    parser.closeFile();
}

static void processHEIF(const std::string& filePath) {
    HEIFParser heifParser;

    if (!heifParser.openFile(filePath)) {
        return;
    }

    if (!heifParser.parseFile()) {
        std::cerr << "Failed to parse HEIF file" << std::endl;
        heifParser.closeFile();
        return;
    }

    heifParser.printInfo();
    heifParser.closeFile();

    // Process extracted NAL units with HEVCParser
    const auto& nalUnits = heifParser.getNALUnits();
    if (nalUnits.empty()) {
        std::cout << "No NAL units found in HEIF file." << std::endl;
        return;
    }

    std::cout << "=== Analyzing " << nalUnits.size() << " Extracted NAL Units ===" << std::endl;
    std::cout << std::endl;

    HEVCParser hevcParser;
    NALUnitParser nalParser;

    for (const auto& heifNalu : nalUnits) {
        NALUnit nalUnit;
        if (heifNalu.data.size() >= 2) {
            nalParser.parseNALUnit(heifNalu.data.data(), heifNalu.data.size(), nalUnit);
            hevcParser.analyzeExtractedNALUnit(nalUnit);
        }
    }

    hevcParser.printSummary();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: hevc_analyzer <file>" << std::endl;
        std::cout << "  Supports raw .hevc bitstream and .heic/.heif container files" << std::endl;
        return 1;
    }

    std::string filePath = argv[1];

    // Auto-detect format: check for ISOBMFF or file extension
    bool isHEIF = false;

    // Check file extension
    const char* ext = strrchr(filePath.c_str(), '.');
    if (ext) {
        if (strcasecmp(ext, ".heic") == 0 || strcasecmp(ext, ".heif") == 0 ||
            strcasecmp(ext, ".hif") == 0 || strcasecmp(ext, ".avif") == 0) {
            isHEIF = true;
        }
    }

    // If extension doesn't tell us, check magic bytes
    if (!isHEIF) {
        isHEIF = HEIFParser::isISOBMFF(filePath);
    }

    if (isHEIF) {
        std::cout << "Detected HEIF/ISOBMFF format" << std::endl;
        processHEIF(filePath);
    } else {
        std::cout << "Detected raw HEVC bitstream format" << std::endl;
        processRawHEVC(filePath);
    }

    return 0;
}
