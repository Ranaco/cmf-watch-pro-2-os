#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
ZEPHYR_WORKSPACE="${ZEPHYR_WORKSPACE:-/home/rana/zephyrproject}"

. "${ZEPHYR_WORKSPACE}/.venv/bin/activate"
cd "${ZEPHYR_WORKSPACE}"

west build -p auto -b native_sim/native/64 \
  "${PROJECT_DIR}/watch" \
  -d "${PROJECT_DIR}/build/simulator" \
  -- -DDTC_OVERLAY_FILE="${PROJECT_DIR}/watch/boards/native_sim.overlay"

exec "${PROJECT_DIR}/build/simulator/zephyr/zephyr.exe"
