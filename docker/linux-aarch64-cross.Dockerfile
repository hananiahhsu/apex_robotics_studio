FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    libc6-dev-arm64-cross \
    qemu-user-static \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

ENV ARS_AARCH64_SYSROOT=/usr/aarch64-linux-gnu
ENV ARS_CROSS_EMULATOR=/usr/bin/qemu-aarch64-static;-L;/usr/aarch64-linux-gnu
WORKDIR /workspace
