#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build/simulator"
ZEPHYR_WORKSPACE="${ZEPHYR_WORKSPACE:-/home/rana/zephyrproject}"

. "${ZEPHYR_WORKSPACE}/.venv/bin/activate"
cd "${ZEPHYR_WORKSPACE}"

west build -p auto -b native_sim/native/64 \
  "${PROJECT_DIR}/watch" \
  -d "${BUILD_DIR}" \
  -- -DDTC_OVERLAY_FILE="${PROJECT_DIR}/watch/boards/native_sim.overlay"

grep -q 'width = < 0x1d2 >' "${BUILD_DIR}/zephyr/zephyr.dts"
grep -q 'height = < 0x1d2 >' "${BUILD_DIR}/zephyr/zephyr.dts"

RUN_LOG="$(mktemp)"
APP_PID=""
cleanup() {
  if [[ -n "${APP_PID}" ]]; then
    kill "${APP_PID}" 2>/dev/null || true
    wait "${APP_PID}" 2>/dev/null || true
  fi
  rm -f "${RUN_LOG}"
}
trap cleanup EXIT

pkill -x zephyr.exe 2>/dev/null || true
"${BUILD_DIR}/zephyr/zephyr.exe" >"${RUN_LOG}" 2>&1 &
APP_PID=$!

WINDOW_ID="$(timeout 5 xdotool search --sync --onlyvisible --name '^Zephyr Display$' | tail -n 1)"
sleep 1

grep -q 'CMF simulator ready platform=native_sim display=466x466 round=1' "${RUN_LOG}"
grep -q "Init 'input-sdl-touch' device" "${RUN_LOG}"

# The R key models the hardware-button fallback and advances the same carousel
# used by a left swipe. Gesture direction and rejection thresholds are covered
# deterministically by test-core.sh because synthetic SDL drags are unreliable
# under headless X servers.
for screen in notifications music assistant settings home; do
  xdotool key --window "${WINDOW_ID}" r
  sleep 1
  grep -q "Navigation: ${screen}" "${RUN_LOG}"
done

[[ "$(grep -c 'Navigation: home' "${RUN_LOG}")" -ge 2 ]]

echo "Simulator smoke check passed: the five-page offline carousel and hardware fallback work."
