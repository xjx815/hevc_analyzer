#include "bit_stream_reader.h"

BitStreamReader::BitStreamReader(const uint8_t* data, size_t size)
    : data_(data), size_(size), pos_(0) {
}

uint8_t BitStreamReader::readBit() {
    if (pos_ / 8 >= size_) {
        return 0;
    }
    uint8_t bit = (data_[pos_ / 8] >> (7 - (pos_ % 8))) & 0x01;
    pos_++;
    return bit;
}

uint32_t BitStreamReader::readBits(uint8_t count) {
    uint32_t result = 0;
    for (uint8_t i = 0; i < count; i++) {
        result = (result << 1) | readBit();
    }
    return result;
}

uint32_t BitStreamReader::readUE() {
    uint8_t leadingZeros = 0;
    while (leadingZeros < 32 && pos_ < size_ * 8 && readBit() == 0) {
        leadingZeros++;
    }
    if (leadingZeros >= 32 || pos_ >= size_ * 8) {
        return 0;
    }
    // Already consumed the leading '1' via readBit() above (the while loop
    // calls readBit() when it encounters the terminating '1', which consumes
    // it AND exits). We do NOT call readBit() again here.
    if (leadingZeros == 0) {
        return 0;
    }
    uint32_t codeNum = readBits(leadingZeros);
    codeNum += (1u << leadingZeros) - 1;
    return codeNum;
}

int32_t BitStreamReader::readSE() {
    uint32_t codeNum = readUE();
    if (codeNum % 2 == 0) {
        return -static_cast<int32_t>(codeNum / 2);
    } else {
        return static_cast<int32_t>((codeNum + 1) / 2);
    }
}

bool BitStreamReader::hasMoreBits() const {
    return pos_ < size_ * 8;
}

size_t BitStreamReader::bitPos() const {
    return pos_;
}

size_t BitStreamReader::totalBits() const {
    return size_ * 8;
}
