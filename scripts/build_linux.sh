#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

cmake --preset linux-ninja-release
cmake --build --preset build-linux-ninja-release
ctest --preset test-linux-ninja-release
cmake --install out/build/linux-ninja-release

echo
echo "Build completed successfully."
echo "Executable: ${ROOT_DIR}/out/build/linux-ninja-release/apex_robotics_studio"
