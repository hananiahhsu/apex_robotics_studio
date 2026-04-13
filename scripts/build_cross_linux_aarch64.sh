#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

: "${ARS_AARCH64_SYSROOT:?ARS_AARCH64_SYSROOT must point to the target sysroot.}"

cmake --preset linux-aarch64-release
cmake --build --preset build-linux-aarch64-release
ctest --preset test-linux-aarch64-release || true
cmake --install out/build/linux-aarch64-release

echo
echo "Cross build completed successfully."
echo "Install root: ${ROOT_DIR}/out/install/linux-aarch64-release"
