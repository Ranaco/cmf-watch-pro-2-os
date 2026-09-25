#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
HOST_BINARY="${PROJECT_DIR}/host/target/debug/cmf-watch-host"
WATCH_BINARY="${PROJECT_DIR}/build/simulator/zephyr/zephyr.exe"
TEMP_DIR="$(mktemp -d)"
CACHE_PATH="${TEMP_DIR}/watch-cache.bin"
HOST_LOG="${TEMP_DIR}/host.log"
ONLINE_LOG="${TEMP_DIR}/watch-online.log"
OFFLINE_LOG="${TEMP_DIR}/watch-offline.log"
HOST_PID=""
WATCH_PID=""

stop_processes() {
  if [[ -n "${WATCH_PID}" ]]; then
    kill "${WATCH_PID}" 2>/dev/null || true
    wait "${WATCH_PID}" 2>/dev/null || true
    WATCH_PID=""
  fi
  if [[ -n "${HOST_PID}" ]]; then
    kill "${HOST_PID}" 2>/dev/null || true
    wait "${HOST_PID}" 2>/dev/null || true
    HOST_PID=""
  fi
}

cleanup() {
  stop_processes
  rm -f "${CACHE_PATH}" "${CACHE_PATH}.tmp" "${HOST_LOG}" "${ONLINE_LOG}" "${OFFLINE_LOG}"
  rmdir "${TEMP_DIR}" 2>/dev/null || true
}
trap cleanup EXIT

[[ -x "${HOST_BINARY}" && -x "${WATCH_BINARY}" ]] || {
  echo "Host or simulator binary is missing; run the normal checks first." >&2
  exit 1
}

"${HOST_BINARY}" >"${HOST_LOG}" 2>&1 &
HOST_PID=$!
CMF_WATCH_CACHE_PATH="${CACHE_PATH}" "${WATCH_BINARY}" >"${ONLINE_LOG}" 2>&1 &
WATCH_PID=$!

for _ in {1..100}; do
  grep -q 'State synchronized revision=4 temp=24 steps=9632 notifications=3' "${ONLINE_LOG}" && break
  sleep 0.1
done
grep -q 'State synchronized revision=4 temp=24 steps=9632 notifications=3' "${ONLINE_LOG}"
[[ -f "${CACHE_PATH}" ]]
[[ "$(stat -c %s "${CACHE_PATH}")" -le 1024 ]]
stop_processes

# Relaunch without a host: the fixed-size record must restore as stale data.
CMF_WATCH_CACHE_PATH="${CACHE_PATH}" "${WATCH_BINARY}" >"${OFFLINE_LOG}" 2>&1 &
WATCH_PID=$!
for _ in {1..50}; do
  grep -q 'CMF simulator ready' "${OFFLINE_LOG}" && break
  sleep 0.1
done
grep -q 'Restored cache revision=4 temp=24 steps=9632 notifications=3 stale=1' "${OFFLINE_LOG}"
grep -q 'CMF simulator ready at round 466x466 (offline=1)' "${OFFLINE_LOG}"

echo "Cache check passed: a bounded record restored synchronized state as stale while offline."
