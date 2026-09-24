# CMF Watch Pro 2 Thin-Client OS

A locally rendered LVGL watch runtime with a transport-independent host architecture. Development starts on Zephyr `native_sim` with SDL; physical-watch work remains behind the documented hardware safety gate.

## Current milestone

Phase 4 complete: round simulator plus layered local runtime and offline navigation.

## Run

From Ubuntu 24.04 in WSL2:

```bash
cd /home/rana/watch
./scripts/run-simulator.sh
```

The simulator renders locally. Click the weather card or press `R` to open Notifications, then use the local Music and Back actions. No host connection is required.

Run all current checks with:

```bash
./scripts/test-core.sh
./scripts/check-simulator.sh
```

## Workspaces

- Project: `/home/rana/watch`
- Zephyr SDK workspace: `/home/rana/zephyrproject`

See `PROJECT_BRIEF.md` for the phased plan and safety gates.
