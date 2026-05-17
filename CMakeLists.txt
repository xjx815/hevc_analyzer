cmake_minimum_required(VERSION 3.10)

project(hevc_analyzer VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_executable(hevc_analyzer
    src/main.cpp
    src/hevc_parser.cpp
    src/nal_unit.cpp
    src/bit_stream_reader.cpp
    src/sps_parser.cpp
    src/pps_parser.cpp
    src/vps_parser.cpp
    src/frame_analyzer.cpp
    src/macroblock_analyzer.cpp
    src/heif_parser.cpp
)

set_target_properties(hevc_analyzer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 平台特定设置
if(APPLE)
    # macOS 特定设置
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
elseif(UNIX AND NOT APPLE)
    # Linux 特定设置
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
elseif(WIN32)
    # Windows 特定设置
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

# 安装配置
install(TARGETS hevc_analyzer
    DESTINATION bin
)
