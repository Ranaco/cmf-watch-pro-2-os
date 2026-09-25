# Development

Primary development runs in WSL2 Ubuntu 24.04. The project is located at `/home/rana/watch`; the isolated Zephyr workspace is `/home/rana/zephyrproject`.

Run the current simulator with:

```bash
cd /home/rana/watch
./scripts/run-simulator.sh
```

Run the localhost host service in a separate terminal with:

```bash
cd /home/rana/watch
cargo run --manifest-path host/Cargo.toml
```

The simulator remains fully usable if the host is absent. When it is present, the Zephyr transport reconnects to `127.0.0.1:4660` and exchanges protocol v1 frames. Use `./scripts/check-transport.sh` for the automated cross-language handshake check.

Set `CMF_HOST_LATENCY_MS` on the host process to inject 0–10,000 ms of response latency. `./scripts/check-latency.sh` verifies synchronization at 100, 300, and 1,000 ms, then launches the simulator without a host and verifies that all five local pages remain navigable.

The normal simulator runner enables persistence at `build/simulator/watch-cache.bin`. Override this with `CMF_WATCH_CACHE_PATH`; omit the variable when launching the binary directly to disable filesystem persistence. `./scripts/check-cache.sh` uses an isolated path to synchronize three notifications, weather, activity, and music, then relaunches without a host and verifies the record restores as stale.
