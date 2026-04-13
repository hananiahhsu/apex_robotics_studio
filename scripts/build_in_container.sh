#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <image-tag> <command...>"
    exit 1
fi

IMAGE_TAG="$1"
shift
ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)

docker run --rm -it \
    -v "${ROOT_DIR}:/workspace" \
    -w /workspace \
    "${IMAGE_TAG}" \
    "$@"
