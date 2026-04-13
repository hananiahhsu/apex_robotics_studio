#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"

cmake --preset linux-to-windows-x64-release
cmake --build --preset build-linux-to-windows-x64-release
cmake --install out/build/linux-to-windows-x64-release

echo
echo "Cross build completed successfully."
echo "Install root: ${ROOT_DIR}/out/install/linux-to-windows-x64-release"
