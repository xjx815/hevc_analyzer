#ifndef BIT_STREAM_READER_H
#define BIT_STREAM_READER_H

#include <stdint.h>
#include <stddef.h>

class BitStreamReader {
public:
    BitStreamReader(const uint8_t* data, size_t size);

    uint8_t readBit();
    uint32_t readBits(uint8_t count);
    uint32_t readUE();       // unsigned Exp-Golomb (ue(v))
    int32_t readSE();        // signed Exp-Golomb (se(v))
    bool hasMoreBits() const;
    size_t bitPos() const;
    size_t totalBits() const;

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;
};

#endif // BIT_STREAM_READER_H
