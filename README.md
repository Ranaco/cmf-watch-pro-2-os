# CMF Watch Pro 2 Thin-Client OS

A locally rendered LVGL watch runtime with a transport-independent host architecture. Development starts on Zephyr `native_sim` with SDL; physical-watch work remains behind the documented hardware safety gate.

## Current milestone

Phase 7 complete: round simulator, layered runtime, five-app offline registry, frozen protocol v1, Rust host service, Zephyr C codec, and localhost TCP transport.

## Run

From Ubuntu 24.04 in WSL2:

```bash
cd /home/rana/watch
./scripts/run-simulator.sh
```

The simulator renders locally. Click the weather card or press `R` to open Notifications, then use the local Music and Back actions. No host connection is required.

To exercise the host connection, start the Rust service in one terminal before launching the simulator:

```bash
cargo run --manifest-path host/Cargo.toml
```

The simulator reconnects automatically to `127.0.0.1:4660` and performs the protocol hello, capabilities, and synchronization sequence. This TCP backend is simulator-only; no BLE or physical-watch access is involved.

Run all current checks with:

```bash
./scripts/test-core.sh
./scripts/check-simulator.sh
./scripts/test-protocol.sh
./scripts/test-host.sh
./scripts/check-transport.sh
```

## Workspaces

- Project: `/home/rana/watch`
- Zephyr SDK workspace: `/home/rana/zephyrproject`

See `PROJECT_BRIEF.md` for the phased plan and safety gates.
