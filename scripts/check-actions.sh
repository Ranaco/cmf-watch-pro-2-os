#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
HOST_BINARY="${PROJECT_DIR}/host/target/debug/cmf-watch-host"
WATCH_BINARY="${PROJECT_DIR}/build/simulator/zephyr/zephyr.exe"
TEMP_DIR="$(mktemp -d)"
HOST_PID=""
WATCH_PID=""

stop_pair() {
  if [[ -n "${WATCH_PID}" ]]; then kill "${WATCH_PID}" 2>/dev/null || true; wait "${WATCH_PID}" 2>/dev/null || true; WATCH_PID=""; fi
  if [[ -n "${HOST_PID}" ]]; then kill "${HOST_PID}" 2>/dev/null || true; wait "${HOST_PID}" 2>/dev/null || true; HOST_PID=""; fi
}
cleanup() {
  stop_pair
  rm -f "${TEMP_DIR}/host.log" "${TEMP_DIR}/watch.log"
  rmdir "${TEMP_DIR}" 2>/dev/null || true
}
trap cleanup EXIT

run_case() {
  local reject="$1"
  local host_log="${TEMP_DIR}/host.log"
  local watch_log="${TEMP_DIR}/watch.log"
  rm -f "${host_log}" "${watch_log}"
  if [[ "${reject}" == "1" ]]; then
    CMF_HOST_REJECT_ACTIONS=1 "${HOST_BINARY}" >"${host_log}" 2>&1 &
  else
    "${HOST_BINARY}" >"${host_log}" 2>&1 &
  fi
  HOST_PID=$!
  "${WATCH_BINARY}" >"${watch_log}" 2>&1 &
  WATCH_PID=$!
  local window_id
  window_id="$(timeout 5 xdotool search --sync --onlyvisible --name '^Zephyr Display$' | tail -n 1)"
  for _ in {1..100}; do
    grep -q 'State synchronized revision=4' "${watch_log}" && break
    sleep 0.1
  done
  grep -q 'State synchronized revision=4' "${watch_log}"
  xdotool key --window "${window_id}" r
  sleep 0.4
  xdotool key --window "${window_id}" r
  sleep 0.4
  grep -q 'Navigation: music' "${watch_log}"
  xdotool key --window "${window_id}" p
  for _ in {1..80}; do
    grep -q 'Action reconciled' "${watch_log}" && break
    sleep 0.1
  done
  if ! grep -q 'Action reconciled' "${watch_log}"; then
    sed -n '1,200p' "${host_log}" >&2
    sed -n '1,260p' "${watch_log}" >&2
    return 1
  fi
  grep -q 'received action' "${host_log}"
  grep -q 'Optimistic music action playing=0' "${watch_log}"
  if [[ "${reject}" == "1" ]]; then
    grep -q 'Action reconciled .* playing=1 feedback=ACTION REJECTED' "${watch_log}"
  else
    grep -q 'Action reconciled .* playing=0 feedback=HOST CONFIRMED' "${watch_log}"
    grep -q 'State synchronized revision=5 .*' "${watch_log}"
  fi
  stop_pair
}

run_case 0
run_case 1

echo "Action check passed: music updates optimistically, confirms on success, and rolls back on rejection."
