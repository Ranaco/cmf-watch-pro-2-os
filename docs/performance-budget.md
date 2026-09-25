# Performance Budget

The simulator milestone uses measurable budgets rather than timing assumptions.

| Metric | Budget | Automated evidence |
| --- | ---: | --- |
| Local navigation render submission | under 50 ms | `check-performance.sh` parses runtime instrumentation |
| Animated transition refresh rate | at least 30 FPS | LVGL refresh-ready events sampled over each 300 ms transition |
| Screen opening | zero host round trips | offline carousel smoke and performance checks |
| Offline startup | must succeed | simulator, latency, cache, and performance checks |
| Reconnection | automatic | transport, latency, and soak checks |
| Protocol frame | at most 16,384 bytes | C and Rust framing tests |
| State/cache memory | statically bounded | fixed C structures and cache record under 1 KiB |

Native-simulator RSS is reported as diagnostic evidence, not treated as the embedded RAM budget because it includes Zephyr native simulation, SDL, LVGL, and host libc. Hardware-specific RAM/flash budgets require a confirmed ATS3089C linker map.

Run:

```bash
./scripts/check-performance.sh
SOAK_SECONDS=3600 ./scripts/soak-test.sh
```

The short soak defaults to 20 seconds for development. The acceptance soak uses 3,600 seconds with periodic host restarts and continuous local navigation/actions.
