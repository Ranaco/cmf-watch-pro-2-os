#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
HOST_BINARY="${PROJECT_DIR}/host/target/debug/cmf-watch-host"
WATCH_BINARY="${PROJECT_DIR}/build/simulator/zephyr/zephyr.exe"
HOST_PID=""
WATCH_PID=""
HOST_LOG=""
WATCH_LOG=""

stop_pair() {
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
  [[ -z "${HOST_LOG}" ]] || rm -f "${HOST_LOG}"
  [[ -z "${WATCH_LOG}" ]] || rm -f "${WATCH_LOG}"
  HOST_LOG=""
  WATCH_LOG=""
}
trap stop_pair EXIT

cargo build --quiet --manifest-path "${PROJECT_DIR}/host/Cargo.toml"
if [[ ! -x "${WATCH_BINARY}" ]]; then
  echo "Simulator binary is missing; run ./scripts/check-simulator.sh first." >&2
  exit 1
fi

for latency in 100 300 1000; do
  HOST_LOG="$(mktemp)"
  WATCH_LOG="$(mktemp)"
  CMF_HOST_LATENCY_MS="${latency}" "${HOST_BINARY}" >"${HOST_LOG}" 2>&1 &
  HOST_PID=$!
  "${WATCH_BINARY}" >"${WATCH_LOG}" 2>&1 &
  WATCH_PID=$!

  for _ in {1..200}; do
    grep -q 'State synchronized revision=4 temp=24 steps=9632 notifications=1' "${WATCH_LOG}" && break
    if ! kill -0 "${HOST_PID}" 2>/dev/null || ! kill -0 "${WATCH_PID}" 2>/dev/null; then
      echo "Latency ${latency} ms run exited before synchronization." >&2
      sed -n '1,160p' "${HOST_LOG}" >&2
      sed -n '1,200p' "${WATCH_LOG}" >&2
      exit 1
    fi
    sleep 0.1
  done

  grep -q "host latency simulation: ${latency} ms" "${HOST_LOG}"
  grep -q 'State synchronized revision=4 temp=24 steps=9632 notifications=1' "${WATCH_LOG}"
  stop_pair
done

# An absent host must not prevent the local carousel from operating.
WATCH_LOG="$(mktemp)"
pkill -x zephyr.exe 2>/dev/null || true
"${WATCH_BINARY}" >"${WATCH_LOG}" 2>&1 &
WATCH_PID=$!
WINDOW_ID="$(timeout 5 xdotool search --sync --onlyvisible --name '^Zephyr Display$' | tail -n 1)"
sleep 0.5
for screen in notifications music assistant settings home; do
  xdotool key --window "${WINDOW_ID}" r
  sleep 0.35
  grep -q "Navigation: ${screen}" "${WATCH_LOG}"
done
grep -q 'CMF simulator ready at round 466x466 (offline=1)' "${WATCH_LOG}"

echo "Latency check passed: synchronization completed at 100, 300, and 1000 ms; the offline carousel stayed local."
