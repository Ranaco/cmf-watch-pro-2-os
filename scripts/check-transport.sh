#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
HOST_LOG="$(mktemp)"
WATCH_LOG="$(mktemp)"
HOST_PID=""
WATCH_PID=""

cleanup() {
  if [[ -n "${WATCH_PID}" ]]; then
    kill "${WATCH_PID}" 2>/dev/null || true
    wait "${WATCH_PID}" 2>/dev/null || true
  fi
  if [[ -n "${HOST_PID}" ]]; then
    kill "${HOST_PID}" 2>/dev/null || true
    wait "${HOST_PID}" 2>/dev/null || true
  fi
  rm -f "${HOST_LOG}" "${WATCH_LOG}"
}
trap cleanup EXIT

cargo build --quiet --manifest-path "${PROJECT_DIR}/host/Cargo.toml"
"${PROJECT_DIR}/host/target/debug/cmf-watch-host" >"${HOST_LOG}" 2>&1 &
HOST_PID=$!

"${PROJECT_DIR}/build/simulator/zephyr/zephyr.exe" >"${WATCH_LOG}" 2>&1 &
WATCH_PID=$!

for _ in {1..100}; do
  if grep -q 'State synchronized revision=4 temp=24 steps=9632 notifications=1' "${WATCH_LOG}"; then
    break
  fi
  if ! kill -0 "${HOST_PID}" 2>/dev/null || ! kill -0 "${WATCH_PID}" 2>/dev/null; then
    echo "Host or simulator exited before synchronization." >&2
    sed -n '1,160p' "${HOST_LOG}" >&2
    sed -n '1,200p' "${WATCH_LOG}" >&2
    exit 1
  fi
  sleep 0.1
done

grep -q 'watch connected' "${HOST_LOG}"
grep -q 'received hello' "${HOST_LOG}"
grep -q 'received sync_request' "${HOST_LOG}"
grep -q 'Connected to host at 127.0.0.1:4660' "${WATCH_LOG}"
grep -q 'Protocol: hello' "${WATCH_LOG}"
grep -q 'Protocol: capabilities' "${WATCH_LOG}"
grep -q 'Protocol: sync_response' "${WATCH_LOG}"
grep -q 'State synchronized revision=1 temp=24 steps=9000 notifications=1' "${WATCH_LOG}"
grep -q 'State synchronized revision=4 temp=24 steps=9632 notifications=1' "${WATCH_LOG}"

echo "Transport integration passed: Rust host and Zephyr watch buffered a pre-snapshot mutation, installed the snapshot atomically, replayed it, and advanced through revision 4."
