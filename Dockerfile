FROM ubuntu:22.04

LABEL maintainer="xjx815"
LABEL description="HEVC Stream Analyzer - A tool for analyzing HEVC/H.265 bitstreams"

# 避免交互式提示
ENV DEBIAN_FRONTEND=noninteractive

# 安装必要的依赖
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /app

# 复制项目文件
COPY . .

# 构建项目
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make

# 创建一个目录用于挂载输入文件
RUN mkdir -p /input

# 设置默认命令
CMD ["./build/bin/hevc_analyzer", "--help"]
