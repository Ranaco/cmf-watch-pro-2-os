#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
HOST_BINARY="${PROJECT_DIR}/host/target/debug/cmf-watch-host"
WATCH_BINARY="${PROJECT_DIR}/build/simulator/zephyr/zephyr.exe"
SOAK_SECONDS="${SOAK_SECONDS:-20}"
SOAK_RECONNECT_SECONDS="${SOAK_RECONNECT_SECONDS:-5}"
TEMP_DIR="$(mktemp -d)"
HOST_LOG="${TEMP_DIR}/host.log"
WATCH_LOG="${TEMP_DIR}/watch.log"
HOST_PID=""
WATCH_PID=""

start_host() {
  "${HOST_BINARY}" >>"${HOST_LOG}" 2>&1 &
  HOST_PID=$!
}

stop_host() {
  if [[ -n "${HOST_PID}" ]]; then
    kill "${HOST_PID}" 2>/dev/null || true
    wait "${HOST_PID}" 2>/dev/null || true
    HOST_PID=""
  fi
}

cleanup() {
  if [[ -n "${WATCH_PID}" ]]; then kill "${WATCH_PID}" 2>/dev/null || true; wait "${WATCH_PID}" 2>/dev/null || true; fi
  stop_host
  rm -f "${HOST_LOG}" "${WATCH_LOG}"
  rmdir "${TEMP_DIR}" 2>/dev/null || true
}
trap cleanup EXIT

start_host
"${WATCH_BINARY}" >"${WATCH_LOG}" 2>&1 &
WATCH_PID=$!
WINDOW_ID="$(timeout 5 xdotool search --sync --onlyvisible --name '^Zephyr Display$' | tail -n 1)"

start_time=${SECONDS}
next_reconnect=$((SECONDS + SOAK_RECONNECT_SECONDS))
page=0
max_watch_rss=0
max_host_rss=0
while (( SECONDS - start_time < SOAK_SECONDS )); do
  kill -0 "${WATCH_PID}"
  xdotool key --window "${WINDOW_ID}" r
  page=$(((page + 1) % 5))
  if (( page == 2 )); then xdotool key --window "${WINDOW_ID}" p; fi
  watch_rss="$(awk '/VmRSS:/ {print $2}' "/proc/${WATCH_PID}/status" 2>/dev/null || echo 0)"
  (( watch_rss > max_watch_rss )) && max_watch_rss=${watch_rss}
  if [[ -n "${HOST_PID}" ]] && kill -0 "${HOST_PID}" 2>/dev/null; then
    host_rss="$(awk '/VmRSS:/ {print $2}' "/proc/${HOST_PID}/status" 2>/dev/null || echo 0)"
    (( host_rss > max_host_rss )) && max_host_rss=${host_rss}
  fi
  if (( SECONDS >= next_reconnect )); then
    stop_host
    sleep 0.25
    start_host
    next_reconnect=$((SECONDS + SOAK_RECONNECT_SECONDS))
  fi
  sleep 0.5
done

for screen in home notifications music assistant settings; do
  grep -q "Navigation: ${screen}" "${WATCH_LOG}"
done
[[ "$(grep -c 'Connected to host at 127.0.0.1:4660' "${WATCH_LOG}")" -ge 2 ]]
! grep -Eq 'Protocol error|FATAL ERROR|ASSERTION FAIL|Segmentation fault' "${WATCH_LOG}"

echo "Soak check passed: ${SOAK_SECONDS}s, reconnect interval ${SOAK_RECONNECT_SECONDS}s, watch max RSS ${max_watch_rss} KiB, host max RSS ${max_host_rss} KiB."
