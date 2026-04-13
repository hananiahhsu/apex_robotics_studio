#!/usr/bin/env bash
set -euo pipefail

if [[ "${EUID}" -ne 0 ]]; then
    echo "Please run with sudo or as root."
    exit 1
fi

apt-get update
apt-get install -y \
    ninja-build \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    libc6-dev-arm64-cross \
    qemu-user-static \
    binutils-mingw-w64-x86-64 \
    gcc-mingw-w64-x86-64 \
    g++-mingw-w64-x86-64

echo "Ubuntu cross-compilation toolchains installed successfully."
