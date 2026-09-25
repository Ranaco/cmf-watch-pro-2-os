# Reverse Engineering

Read-only reverse-engineering research began after the simulator milestone. Claims are recorded as `CONFIRMED`, `LIKELY`, or `UNKNOWN`, with pinned source revisions in `research/sources.md`.

## Community reference

The community repository <https://github.com/whatotter/cmf-watch-firmware> is retained as a research reference only. Its README describes a firmware dump, extracted image archives, and a large strings catalog; it is not a maintainable UI source tree. The strings catalog contains vocabulary for weather, notifications, music, health, alarms, timers, workouts, and settings, which may inform future feature naming.

The repository's Zephyr, LVGL, and ATS3089C observations are treated as community evidence and cross-checked against the separate Actions reference. No binary, extracted proprietary asset, recompiler, flashing method, or hardware-access code from that repository is imported or executed by this project.

The unofficial BLE protocol repository is useful because it distinguishes live-device validation, firmware/APK findings, partial evidence, and uncertainty. Its claims remain external research; no GATT scan, pairing, secret retrieval, command, transfer, or OTA operation has been performed.
