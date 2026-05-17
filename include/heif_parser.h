#ifndef HEIF_PARSER_H
#define HEIF_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <string>
#include <map>
#include <utility>

// Represents a single NAL unit extracted from HEIF
struct HEIFNALUnit {
    std::vector<uint8_t> data;
    uint8_t nal_unit_type;
};

// ISOBMFF box info
struct BoxInfo {
    uint32_t size;
    uint32_t largesize;
    char type[5];  // 4 chars + null terminator
    uint64_t data_offset;
    uint64_t data_size;
};

class HEIFParser {
public:
    HEIFParser();

    bool openFile(const std::string& filePath);
    void closeFile();

    // Parse the HEIF structure and extract NAL units
    // Returns true if this is a valid HEIF/ISOBMFF file
    bool parseFile();

    // Get all extracted NAL units (parameter sets from hvcC + samples from mdat)
    const std::vector<HEIFNALUnit>& getNALUnits() const { return nalUnits_; }

    // Print parsed HEIF structure info
    void printInfo() const;

    // Print the full box tree
    void printBoxTree() const;

    // Static helper: check if a file is ISOBMFF format by reading the first bytes
    static bool isISOBMFF(const std::string& filePath);

private:
    FILE* fp_;
    bool fileOpened_;

    // Read big-endian uint32/64 from file at current position
    uint32_t readUint32();
    uint64_t readUint64();

    // Parse a box header at current file position. Returns true if more boxes follow.
    bool parseBoxHeader(BoxInfo& box);

    // Recursively parse boxes starting at offset with total size
    void parseBoxes(uint64_t startOffset, uint64_t boxSize, int depth = 0);

    // Parse specific box types
    void parseFtypBox(const BoxInfo& box);
    void parseHvcCBox(const BoxInfo& box);
    void parseStcoBox(const BoxInfo& box);
    void parseCo64Box(const BoxInfo& box);
    void parseStszBox(const BoxInfo& box);
    void parseMetaBox(const BoxInfo& box);

    // Read NAL unit arrays from hvcC box
    void parseNALUnitArrays(const uint8_t* data, size_t size);

    // Extract NAL units from mdat using chunk offsets and sample sizes
    void extractMdatNALUnits();

    // File type info
    char majorBrand_[5];
    uint32_t minorVersion_;
    std::vector<std::string> compatibleBrands_;

    // HEVC configuration
    uint8_t lengthSizeMinusOne_;  // NALU length field size (typically 3 = 4 bytes)
    uint8_t numOfArrays_;

    // Chunk offsets (from stco or co64)
    std::vector<uint64_t> chunkOffsets_;

    // Sample sizes (from stsz)
    uint32_t sampleSize_;         // 0 means variable sizes
    std::vector<uint32_t> sampleSizes_;

    // mdat offset and size
    uint64_t mdatOffset_;
    uint64_t mdatSize_;

    // Extracted NAL units
    std::vector<HEIFNALUnit> nalUnits_;

    // Box tree: (depth, BoxInfo) pairs in traversal order
    std::vector<std::pair<int, BoxInfo>> boxTree_;

    // Track whether we found HEVC data
    bool hasHEVCData_;
};

#endif // HEIF_PARSER_H
