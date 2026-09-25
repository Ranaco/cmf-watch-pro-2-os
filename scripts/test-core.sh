#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build/tests"

mkdir -p "${BUILD_DIR}"
gcc -std=c17 -Wall -Wextra -Werror \
  -I"${PROJECT_DIR}/watch/src" \
  "${PROJECT_DIR}/watch/src/navigation/navigation.c" \
  "${PROJECT_DIR}/watch/src/input/watch_gesture.c" \
  "${PROJECT_DIR}/watch/src/cache/watch_cache.c" \
  "${PROJECT_DIR}/watch/src/state/watch_state.c" \
  "${PROJECT_DIR}/watch/src/apps/app_registry.c" \
  "${PROJECT_DIR}/watch/tests/navigation_test.c" \
  -o "${BUILD_DIR}/core-tests"

"${BUILD_DIR}/core-tests"
