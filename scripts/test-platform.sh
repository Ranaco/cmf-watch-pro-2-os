#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build/tests"

mkdir -p "${BUILD_DIR}"
gcc -std=c17 -Wall -Wextra -Werror \
	-I"${PROJECT_DIR}/watch/src" \
	"${PROJECT_DIR}/watch/src/platform/watch_platform_native.c" \
	"${PROJECT_DIR}/watch/tests/platform_test.c" \
	-o "${BUILD_DIR}/platform-tests"

"${BUILD_DIR}/platform-tests"
