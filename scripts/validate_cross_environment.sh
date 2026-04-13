#!/usr/bin/env bash
set -euo pipefail

function check_tool() {
    local name="$1"
    if command -v "$name" >/dev/null 2>&1; then
        echo "[OK] Found tool: $name -> $(command -v "$name")"
    else
        echo "[WARN] Missing tool: $name"
    fi
}

echo "=== ApexRoboticsStudio Cross Environment Validation ==="
check_tool cmake
check_tool ninja
check_tool aarch64-linux-gnu-gcc
check_tool aarch64-linux-gnu-g++
check_tool x86_64-w64-mingw32-gcc
check_tool x86_64-w64-mingw32-g++
check_tool qemu-aarch64-static

if [[ -n "${ARS_AARCH64_SYSROOT:-}" ]]; then
    if [[ -d "${ARS_AARCH64_SYSROOT}" ]]; then
        echo "[OK] ARS_AARCH64_SYSROOT=${ARS_AARCH64_SYSROOT}"
    else
        echo "[WARN] ARS_AARCH64_SYSROOT points to a missing directory: ${ARS_AARCH64_SYSROOT}"
    fi
else
    echo "[INFO] ARS_AARCH64_SYSROOT is not set"
fi
