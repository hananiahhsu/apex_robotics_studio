FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    gcc-mingw-w64-x86-64 \
    g++-mingw-w64-x86-64 \
    mingw-w64 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
