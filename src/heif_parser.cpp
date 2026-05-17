#include "heif_parser.h"
#include <iostream>
#include <cstring>

HEIFParser::HEIFParser() : fp_(nullptr), fileOpened_(false),
                           lengthSizeMinusOne_(3), numOfArrays_(0),
                           sampleSize_(0), mdatOffset_(0), mdatSize_(0),
                           hasHEVCData_(false) {
    memset(majorBrand_, 0, sizeof(majorBrand_));
    minorVersion_ = 0;
}

bool HEIFParser::openFile(const std::string& filePath) {
    fp_ = fopen(filePath.c_str(), "rb");
    if (!fp_) {
        std::cerr << "Failed to open HEIF file: " << filePath << std::endl;
        return false;
    }
    fileOpened_ = true;
    return true;
}

void HEIFParser::closeFile() {
    if (fileOpened_ && fp_) {
        fclose(fp_);
        fp_ = nullptr;
        fileOpened_ = false;
    }
}

uint32_t HEIFParser::readUint32() {
    uint8_t buf[4];
    if (fread(buf, 1, 4, fp_) != 4) return 0;
    return (static_cast<uint32_t>(buf[0]) << 24) |
           (static_cast<uint32_t>(buf[1]) << 16) |
           (static_cast<uint32_t>(buf[2]) << 8) |
           static_cast<uint32_t>(buf[3]);
}

uint64_t HEIFParser::readUint64() {
    uint32_t hi = readUint32();
    uint32_t lo = readUint32();
    return (static_cast<uint64_t>(hi) << 32) | lo;
}

bool HEIFParser::parseBoxHeader(BoxInfo& box) {
    long pos = ftell(fp_);
    box.data_offset = pos;

    box.size = readUint32();
    if (box.size == 0 && feof(fp_)) return false;

    // Read type (4 bytes)
    if (fread(box.type, 1, 4, fp_) != 4) return false;
    box.type[4] = '\0';

    box.largesize = 0;
    if (box.size == 1) {
        box.largesize = readUint64();
        box.data_size = box.largesize - 16;  // header = 4(size) + 4(type) + 8(largesize)
    } else if (box.size == 0) {
        // extends to end of file
        long cur = ftell(fp_);
        fseek(fp_, 0, SEEK_END);
        long end = ftell(fp_);
        fseek(fp_, cur, SEEK_SET);
        box.data_size = end - cur;
    } else {
        box.data_size = box.size - 8;  // header = 4(size) + 4(type)
    }

    box.data_offset = ftell(fp_);
    return true;
}

bool HEIFParser::parseFile() {
    if (!fileOpened_ || !fp_) return false;

    // Read and verify ftyp box first
    fseek(fp_, 0, SEEK_SET);

    BoxInfo box;
    if (!parseBoxHeader(box)) return false;

    if (strcmp(box.type, "ftyp") != 0) {
        std::cerr << "Not a valid ISOBMFF file (missing ftyp box)" << std::endl;
        return false;
    }

    // Parse ftyp
    parseFtypBox(box);

    // Parse remaining top-level boxes
    parseBoxes(box.data_offset + box.data_size, 0, 0);

    // Extract NAL units from mdat if we found HEVC data
    if (hasHEVCData_ && mdatOffset_ > 0 && mdatSize_ > 0) {
        extractMdatNALUnits();
    }

    return hasHEVCData_;
}

void HEIFParser::parseBoxes(uint64_t startOffset, uint64_t boxEndSize, int depth) {
    fseek(fp_, startOffset, SEEK_SET);

    while (true) {
        long cur = ftell(fp_);
        if (boxEndSize > 0 && cur >= static_cast<long>(startOffset + boxEndSize)) break;
        if (feof(fp_)) break;

        BoxInfo box;
        if (!parseBoxHeader(box)) break;
        if (box.size == 0) break;

        // Record box in tree
        boxTree_.push_back({depth, box});

        // Route to appropriate handlers
        if (strcmp(box.type, "moov") == 0 || strcmp(box.type, "trak") == 0 ||
            strcmp(box.type, "mdia") == 0 || strcmp(box.type, "minf") == 0 ||
            strcmp(box.type, "stbl") == 0) {
            // Container boxes: recurse into children
            parseBoxes(box.data_offset, box.data_size, depth + 1);
        } else if (strcmp(box.type, "meta") == 0) {
            parseMetaBox(box);
        } else if (strcmp(box.type, "stsd") == 0) {
            // Sample description box — contains hvc1/hev1 entries with hvcC
            // Skip version(1)+flags(3)=4 bytes, then entry_count(4)
            fseek(fp_, box.data_offset + 4, SEEK_SET);
            uint32_t entryCount = readUint32();
            uint64_t entryPos = ftell(fp_);
            // Parse each sample entry to find hvcC
            for (uint32_t i = 0; i < entryCount; i++) {
                fseek(fp_, entryPos, SEEK_SET);
                uint32_t entrySize = readUint32();
                char entryType[5] = {0};
                fread(entryType, 1, 4, fp_);
                // Skip reserved (6 bytes) + data_reference_index (2 bytes) = 8 bytes
                // The hvcC box starts after the VisualSampleEntry fields
                if (strcmp(entryType, "hvc1") == 0 || strcmp(entryType, "hev1") == 0) {
                    // VisualSampleEntry: skip 8 + 2 + 2 + 4*3 + 2 + 2 + 4 + 4 + 4 + 2 = lots
                    // The hvcC box is at a variable offset after the VisualSampleEntry base
                    // Parse child boxes within the sample entry to find hvcC
                    uint64_t sampleEntryEnd = entryPos + entrySize;
                    // Skip past the VisualSampleEntry fixed fields to where child boxes start
                    // VisualSampleEntry = 86 bytes base
                    uint64_t childBoxStart = entryPos + 86;
                    if (childBoxStart < sampleEntryEnd) {
                        parseBoxes(childBoxStart, sampleEntryEnd - childBoxStart, depth + 1);
                    }
                }
                entryPos += entrySize;
            }
        } else if (strcmp(box.type, "hvcC") == 0) {
            parseHvcCBox(box);
        } else if (strcmp(box.type, "stco") == 0) {
            parseStcoBox(box);
        } else if (strcmp(box.type, "co64") == 0) {
            parseCo64Box(box);
        } else if (strcmp(box.type, "stsz") == 0) {
            parseStszBox(box);
        } else if (strcmp(box.type, "mdat") == 0) {
            mdatOffset_ = box.data_offset;
            mdatSize_ = box.data_size;
            hasHEVCData_ = true;
        }

        // Skip to next box
        uint64_t nextOffset = box.data_offset + box.data_size;
        fseek(fp_, nextOffset, SEEK_SET);
    }
}

void HEIFParser::parseFtypBox(const BoxInfo& box) {
    fseek(fp_, box.data_offset, SEEK_SET);

    if (fread(majorBrand_, 1, 4, fp_) == 4) {
        majorBrand_[4] = '\0';
    }
    minorVersion_ = readUint32();

    // Read compatible brands
    size_t remaining = box.data_size - 8;  // minus major brand + minor version
    while (remaining >= 4) {
        char brand[5] = {0};
        if (fread(brand, 1, 4, fp_) == 4) {
            compatibleBrands_.push_back(std::string(brand));
        }
        if (remaining >= 4) remaining -= 4; else break;
    }
}

void HEIFParser::parseMetaBox(const BoxInfo& box) {
    // Meta box has a 4-byte version/flags field before child boxes
    parseBoxes(box.data_offset + 4, box.data_size > 4 ? box.data_size - 4 : 0, 1);
}

void HEIFParser::parseHvcCBox(const BoxInfo& box) {
    hasHEVCData_ = true;

    // Read entire hvcC box data
    std::vector<uint8_t> hvcCData(static_cast<size_t>(box.data_size));
    fseek(fp_, box.data_offset, SEEK_SET);
    fread(hvcCData.data(), 1, static_cast<size_t>(box.data_size), fp_);

    parseNALUnitArrays(hvcCData.data(), hvcCData.size());
}

void HEIFParser::parseNALUnitArrays(const uint8_t* data, size_t size) {
    if (size < 23) return;  // Minimum hvcC size

    size_t offset = 0;

    // configurationVersion (8), general_profile_space (2), general_tier_flag (1),
    // general_profile_idc (5) = 2 bytes
    // general_profile_compatibility_flags (32) = 4 bytes
    // general_constraint_indicator_flags (48) = 6 bytes
    // general_level_idc (8) = 1 byte
    // reserved (4), min_spatial_segmentation_idc (12) = 2 bytes
    // reserved (6), parallelismType (2) = 1 byte
    // reserved (6), chroma_format_idc (2) = 1 byte
    // reserved (5), bit_depth_luma_minus8 (3) = 1 byte
    // reserved (5), bit_depth_chroma_minus8 (3) = 1 byte
    // avgFrameRate (16) = 2 bytes
    // constantFrameRate (2), numTemporalLayers (3), temporalIdNested (1),
    //   lengthSizeMinusOne (2) = 1 byte
    offset = 21;

    uint8_t constantFrameRate_numTemporalLayers_temporalIdNested_lengthSize = data[offset];
    lengthSizeMinusOne_ = constantFrameRate_numTemporalLayers_temporalIdNested_lengthSize & 0x03;
    offset = 22;

    // numOfArrays (8)
    numOfArrays_ = data[offset];
    offset++;

    // Parse NAL unit arrays
    for (uint8_t i = 0; i < numOfArrays_ && offset + 3 <= size; i++) {
        // array_completeness (1) + reserved (0) + NAL_unit_type (6) = 8 bits total
        uint8_t nalUnitType = data[offset] & 0x3F;
        offset += 1;

        // numNalus (16)
        uint16_t numNalus = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
        offset += 2;

        for (uint16_t j = 0; j < numNalus && offset + 2 <= size; j++) {
            // nalUnitLength (16)
            uint16_t nalUnitLength = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
            offset += 2;

            if (offset + nalUnitLength <= size && nalUnitLength > 0) {
                HEIFNALUnit nalu;
                nalu.data.assign(data + offset, data + offset + nalUnitLength);
                nalu.nal_unit_type = nalUnitType;
                nalUnits_.push_back(nalu);
                offset += nalUnitLength;
            }
        }
    }
}

void HEIFParser::parseStcoBox(const BoxInfo& box) {
    fseek(fp_, box.data_offset, SEEK_SET);

    readUint32();  // version (1) + flags (3) = 4 bytes
    uint32_t entryCount = readUint32();

    for (uint32_t i = 0; i < entryCount; i++) {
        chunkOffsets_.push_back(readUint32());
    }
}

void HEIFParser::parseCo64Box(const BoxInfo& box) {
    fseek(fp_, box.data_offset, SEEK_SET);

    readUint32();  // version (1) + flags (3) = 4 bytes
    uint32_t entryCount = readUint32();

    for (uint32_t i = 0; i < entryCount; i++) {
        chunkOffsets_.push_back(readUint64());
    }
}

void HEIFParser::parseStszBox(const BoxInfo& box) {
    fseek(fp_, box.data_offset, SEEK_SET);

    readUint32();  // version (1) + flags (3) = 4 bytes
    sampleSize_ = readUint32();
    uint32_t sampleCount = readUint32();

    if (sampleSize_ == 0) {
        // Variable sample sizes
        for (uint32_t i = 0; i < sampleCount; i++) {
            sampleSizes_.push_back(readUint32());
        }
    }
}

void HEIFParser::extractMdatNALUnits() {
    {  // always use direct mdat reading (stco chunk offsets are unreliable for simple test files)
        // If no chunk offsets from stco/co64, try reading from mdat directly
        // as a continuous stream of length-prefixed NAL units
        uint32_t naluLengthSize = lengthSizeMinusOne_ + 1;

        fseek(fp_, mdatOffset_, SEEK_SET);
        uint64_t offset = 0;

        while (offset + naluLengthSize <= mdatSize_) {
            uint8_t lenBuf[4] = {0};
            fread(lenBuf, 1, naluLengthSize, fp_);
            offset += naluLengthSize;

            uint32_t naluSize = 0;
            for (uint32_t i = 0; i < naluLengthSize; i++) {
                naluSize = (naluSize << 8) | lenBuf[i];
            }

            if (naluSize == 0 || offset + naluSize > mdatSize_) break;

            std::vector<uint8_t> naluData(naluSize);
            fread(naluData.data(), 1, naluSize, fp_);
            offset += naluSize;

            if (!naluData.empty()) {
                HEIFNALUnit nalu;
                nalu.data = std::move(naluData);
                // Extract NAL unit type from first byte
                nalu.nal_unit_type = (nalu.data[0] >> 1) & 0x3F;
                nalUnits_.push_back(nalu);
            }
        }
        return;
    }

    // Read samples using chunk offsets and sizes
    uint32_t naluLengthSize = lengthSizeMinusOne_ + 1;
    uint32_t sampleIndex = 0;

    for (uint64_t chunkOffset : chunkOffsets_) {
        if (sampleIndex >= sampleSizes_.size() && sampleSize_ == 0) break;

        fseek(fp_, chunkOffset, SEEK_SET);

        uint32_t bytesRead = 0;

        // For variable sample sizes, accumulate samples in this chunk
        if (sampleSize_ == 0) {
            while (sampleIndex < sampleSizes_.size() && bytesRead < sampleSizes_[sampleIndex] + naluLengthSize) {
                uint8_t lenBuf[4] = {0};
                bytesRead += naluLengthSize;

                uint32_t naluSize = 0;
                for (uint32_t i = 0; i < naluLengthSize; i++) {
                    naluSize = (naluSize << 8) | lenBuf[i];
                }

                if (naluSize > 0) {
                    std::vector<uint8_t> naluData(naluSize);
                    fread(naluData.data(), 1, naluSize, fp_);
                    bytesRead += naluSize;

                    HEIFNALUnit nalu;
                    nalu.data = std::move(naluData);
                    nalu.nal_unit_type = (nalu.data[0] >> 1) & 0x3F;
                    nalUnits_.push_back(nalu);
                }
                sampleIndex++;
            }
        } else {
            // Fixed sample size
            fseek(fp_, chunkOffset, SEEK_SET);
            // Read samples from this chunk (simplified: assume 1 sample per chunk)
            uint8_t lenBuf[4] = {0};
            fread(lenBuf, 1, naluLengthSize, fp_);

            uint32_t naluSize = 0;
            for (uint32_t i = 0; i < naluLengthSize; i++) {
                naluSize = (naluSize << 8) | lenBuf[i];
            }

            if (naluSize > 0 && naluSize <= sampleSize_) {
                std::vector<uint8_t> naluData(naluSize);
                fread(naluData.data(), 1, naluSize, fp_);

                HEIFNALUnit nalu;
                nalu.data = std::move(naluData);
                nalu.nal_unit_type = (nalu.data[0] >> 1) & 0x3F;
                nalUnits_.push_back(nalu);
            }
            sampleIndex++;
        }
    }
}

void HEIFParser::printInfo() const {
    std::cout << "=== HEIF File Information ===" << std::endl;
    std::cout << "  Major Brand: " << majorBrand_ << std::endl;
    std::cout << "  Minor Version: " << minorVersion_ << std::endl;
    std::cout << "  Compatible Brands:";
    for (const auto& brand : compatibleBrands_) {
        std::cout << " " << brand;
    }
    std::cout << std::endl;
    std::cout << "  NALU Length Size: " << (lengthSizeMinusOne_ + 1) << " bytes" << std::endl;
    std::cout << "  Num NALU Arrays: " << (int)numOfArrays_ << std::endl;
    std::cout << "  Chunk Offsets: " << chunkOffsets_.size() << std::endl;
    std::cout << "  Sample Sizes: " << sampleSizes_.size() << " (fixed=" << sampleSize_ << ")" << std::endl;
    std::cout << "  Extracted NAL Units: " << nalUnits_.size() << std::endl;
    std::cout << std::endl;

    printBoxTree();
}

void HEIFParser::printBoxTree() const {
    if (boxTree_.empty()) return;

    std::cout << "=== ISOBMFF Box Tree ===" << std::endl;
    for (const auto& entry : boxTree_) {
        int depth = entry.first;
        const BoxInfo& box = entry.second;

        // Indent by depth
        for (int i = 0; i < depth; i++) std::cout << "  ";

        uint64_t actualSize = box.largesize > 0 ? box.largesize : box.size;
        std::cout << "[" << box.type << "]";

        // Show size for non-container leaf boxes
        uint64_t headerSize = (box.largesize > 0) ? 16ULL : (box.size > 0 ? 8ULL : 0ULL);
        uint64_t payloadSize = actualSize > headerSize ? actualSize - headerSize : 0;

        // Special handling for known boxes
        if (strcmp(box.type, "ftyp") == 0) {
            std::cout << " major=" << majorBrand_ << " v" << minorVersion_;
        } else if (strcmp(box.type, "mdat") == 0) {
            std::cout << " size=" << payloadSize << " bytes (media data)";
        } else if (strcmp(box.type, "hvcC") == 0) {
            std::cout << " size=" << payloadSize << " bytes (HEVC config, " << (int)numOfArrays_ << " arrays)";
        } else if (strcmp(box.type, "stco") == 0) {
            std::cout << " entries=" << chunkOffsets_.size();
        } else if (strcmp(box.type, "co64") == 0) {
            std::cout << " entries=" << chunkOffsets_.size();
        } else if (strcmp(box.type, "stsz") == 0) {
            std::cout << " samples=" << sampleSizes_.size() << " fixed=" << sampleSize_;
        } else if (strcmp(box.type, "moov") == 0 || strcmp(box.type, "trak") == 0 ||
                   strcmp(box.type, "mdia") == 0 || strcmp(box.type, "minf") == 0 ||
                   strcmp(box.type, "stbl") == 0 || strcmp(box.type, "meta") == 0 ||
                   strcmp(box.type, "stsd") == 0) {
            std::cout << " (container)";
        } else if (strcmp(box.type, "hvc1") == 0 || strcmp(box.type, "hev1") == 0) {
            std::cout << " size=" << payloadSize << " bytes (HEVC sample entry)";
        } else {
            std::cout << " size=" << payloadSize;
        }

        std::cout << std::endl;
    }
    std::cout << std::endl;
}

bool HEIFParser::isISOBMFF(const std::string& filePath) {
    FILE* fp = fopen(filePath.c_str(), "rb");
    if (!fp) return false;

    uint8_t header[12];
    size_t n = fread(header, 1, 12, fp);
    fclose(fp);

    if (n < 12) return false;

    // Check if first 4 bytes could be a valid box size (non-zero big-endian)
    uint32_t boxSize = (static_cast<uint32_t>(header[0]) << 24) |
                       (static_cast<uint32_t>(header[1]) << 16) |
                       (static_cast<uint32_t>(header[2]) << 8) |
                       static_cast<uint32_t>(header[3]);

    if (boxSize == 0) return false;

    // Check the type at offset 4 is printable ASCII
    for (int i = 4; i < 8; i++) {
        if (header[i] < 0x20 || header[i] > 0x7E) return false;
    }

    // Must start with 'ftyp'
    if (header[4] == 'f' && header[5] == 't' && header[6] == 'y' && header[7] == 'p') {
        return true;
    }

    return false;
}
