#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR="${ROOT_DIR}/out/build/linux-ninja-release"

cmake --preset linux-ninja-release
cmake --build --preset build-linux-ninja-release
ctest --preset test-linux-ninja-release --output-on-failure
cpack --config "${BUILD_DIR}/CPackConfig.cmake"
