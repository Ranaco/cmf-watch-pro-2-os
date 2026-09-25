#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
WATCH_BINARY="${PROJECT_DIR}/build/simulator/zephyr/zephyr.exe"
RUN_LOG="$(mktemp)"
WATCH_PID=""

cleanup() {
  if [[ -n "${WATCH_PID}" ]]; then kill "${WATCH_PID}" 2>/dev/null || true; wait "${WATCH_PID}" 2>/dev/null || true; fi
  rm -f "${RUN_LOG}"
}
trap cleanup EXIT

pkill -x zephyr.exe 2>/dev/null || true
"${WATCH_BINARY}" >"${RUN_LOG}" 2>&1 &
WATCH_PID=$!
WINDOW_ID="$(timeout 5 xdotool search --sync --onlyvisible --name '^Zephyr Display$' | tail -n 1)"
sleep 0.5
for _ in {1..10}; do
  xdotool key --window "${WINDOW_ID}" r
  sleep 0.4
done

grep -q 'CMF simulator ready platform=native_sim display=466x466 round=1 offline=1' "${RUN_LOG}"
render_max="$(sed -n 's/.*PERF navigation_render_ms=\([0-9][0-9]*\).*/\1/p' "${RUN_LOG}" | sort -nr | head -n 1)"
fps_min="$(sed -n 's/.*PERF animation_frames=[0-9][0-9]* window_ms=300 fps=\([0-9][0-9]*\).*/\1/p' "${RUN_LOG}" | sort -n | head -n 1)"
if [[ -z "${render_max}" || -z "${fps_min}" ]]; then
  sed -n '1,260p' "${RUN_LOG}" >&2
  exit 1
fi
[[ -n "${render_max}" && "${render_max}" -lt 50 ]]
if [[ "${fps_min}" -lt 30 ]]; then
  sed -n '1,260p' "${RUN_LOG}" >&2
  exit 1
fi
rss_kib="$(awk '/VmRSS:/ {print $2}' "/proc/${WATCH_PID}/status")"

echo "Performance check passed: max local render ${render_max} ms, minimum sampled animation ${fps_min} FPS, native simulator RSS ${rss_kib} KiB."
