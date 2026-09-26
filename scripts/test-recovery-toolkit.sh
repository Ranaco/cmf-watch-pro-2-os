#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

python3 -m unittest discover \
  -s "${PROJECT_DIR}/recovery/tests" \
  -p 'test_*.py' \
  -v

python3 "${PROJECT_DIR}/tools/recovery_tool.py" --help >/dev/null
echo "Recovery toolkit tests passed."
