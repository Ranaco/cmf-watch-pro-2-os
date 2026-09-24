#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
ZEPHYR_WORKSPACE="${ZEPHYR_WORKSPACE:-/home/rana/zephyrproject}"
. "${ZEPHYR_WORKSPACE}/.venv/bin/activate"
python "${PROJECT_DIR}/protocol/tests/test_protocol.py"
