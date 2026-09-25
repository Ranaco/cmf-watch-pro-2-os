# CMF Watch Pro 2 Thin-Client OS

A locally rendered LVGL watch runtime with a transport-independent host architecture. Development starts on Zephyr `native_sim` with SDL; physical-watch work remains behind the documented hardware safety gate.

## Current milestone

Phase 8 complete: round simulator, layered runtime, five-page offline carousel, frozen protocol v1, Rust host service, Zephyr C codec, localhost TCP transport, and atomic host-state synchronization into the local UI.

## Run

From Ubuntu 24.04 in WSL2:

```bash
cd /home/rana/watch
./scripts/run-simulator.sh
```

The simulator renders locally. Swipe left or right anywhere on the round face to move through Home, Notifications, Music, Assistant, and Settings. The simulated hardware button (`R`) advances to the next page as a non-touch fallback. There are no on-screen navigation buttons, and no host connection is required.

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

Run all current checks with:

```bash
./scripts/test-core.sh
./scripts/check-simulator.sh
./scripts/test-protocol.sh
./scripts/test-host.sh
./scripts/check-transport.sh
./scripts/check-latency.sh
```

## Workspaces

- Project: `/home/rana/watch`
- Zephyr SDK workspace: `/home/rana/zephyrproject`

See `PROJECT_BRIEF.md` for the phased plan and safety gates.
