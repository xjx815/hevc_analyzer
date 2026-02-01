#ifndef NAL_UNIT_H
#define NAL_UNIT_H

#include "hevc_types.h"

class NALUnitParser {
public:
    NALUnitParser();
    ~NALUnitParser();
    
    // 解析 NAL 单元头
    bool parseNALUnitHeader(const uint8_t* data, size_t size, NALUnit& nalUnit);
    
    // 解析完整的 NAL 单元
    bool parseNALUnit(const uint8_t* data, size_t size, NALUnit& nalUnit);
    
    // 获取 NAL 单元类型的字符串表示
    std::string getNALUnitTypeString(uint8_t nalUnitType);
    
    // 输出 NAL 单元信息
    void printNALUnitInfo(const NALUnit& nalUnit);
};

#endif // NAL_UNIT_H
