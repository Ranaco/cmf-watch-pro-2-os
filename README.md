# CMF Watch Pro 2 Thin-Client OS

A locally rendered LVGL watch runtime with a transport-independent host architecture. Development starts on Zephyr `native_sim` with SDL; physical-watch work remains behind the documented hardware safety gate.

## Current milestone

Simulator product milestone complete through Phase 14, followed by the read-only platform milestone through Phase 19: offline-first round UI, frozen protocol v1, Rust host, atomic synchronization, bounded cache, optimistic actions, compiled app manifests, reconnect soak coverage, measured performance budgets, a compile-proven Actions reference, a platform capability contract, and an evidence-gated CMF board scaffold.

## Run

From Ubuntu 24.04 in WSL2:

```bash
cd /home/rana/watch
./scripts/run-simulator.sh
```

The simulator renders locally. Swipe left or right anywhere on the round face to move through Home, Notifications, Music, Assistant, and Settings. Swipe vertically on Notifications to browse up to four cached entries. Tap the Music page to play/pause optimistically. The simulated `R` key advances to the next page and `P` activates the current page as hardware-input fallbacks. There are no on-screen navigation buttons, and ordinary navigation requires no host.

To exercise the host connection, start the Rust service in one terminal before launching the simulator:

```bash
cargo run --manifest-path host/Cargo.toml
```

The simulator reconnects automatically to `127.0.0.1:4660`, performs the protocol hello/capabilities/synchronization sequence, and locally renders synchronized weather, steps, notifications, and music state. This TCP backend is simulator-only; no BLE or physical-watch access is involved.

Development latency can be injected without blocking the watch runtime:

```bash
CMF_HOST_LATENCY_MS=1000 cargo run --manifest-path host/Cargo.toml
```

Accepted values are 0–10,000 milliseconds. The Phase 9 matrix covers 100 ms, 300 ms, 1,000 ms, and fully offline operation.

`run-simulator.sh` stores a fixed-size, checksummed cache at `build/simulator/watch-cache.bin` by default. Set `CMF_WATCH_CACHE_PATH` to override it. Restored host data is visibly marked `CACHED` or `CACHE / STALE` until synchronization succeeds.

Run all current checks with:

```bash
./scripts/test-core.sh
./scripts/test-platform.sh
./scripts/check-simulator.sh
./scripts/test-protocol.sh
./scripts/test-host.sh
./scripts/check-transport.sh
./scripts/check-latency.sh
./scripts/check-cache.sh
./scripts/test-app-manifests.sh
./scripts/check-actions.sh
./scripts/check-performance.sh
SOAK_SECONDS=20 ./scripts/soak-test.sh
```

Use `SOAK_SECONDS=3600 ./scripts/soak-test.sh` for the one-hour acceptance soak.

## Workspaces

- Project: `/home/rana/watch`
- Zephyr SDK workspace: `/home/rana/zephyrproject`

See `PROJECT_BRIEF.md` for the phased plan and safety gates.
