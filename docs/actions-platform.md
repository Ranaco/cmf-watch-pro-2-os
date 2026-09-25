# Actions platform research

## Result

The pinned Actions/LVGL reference can compile an application for its `ats3089c_dev_watch` target in WSL. This proves that the published vendor tree, its Zephyr 2.7 fork, its `leopard` SoC support, and its evaluation-board definition form a compilable application platform. It does **not** prove that the retail CMF board shares the EVB's pin map, memory map, panel, touch controller, radio setup, storage layout, or boot chain.

## Compile-only proof

Source commit: `26f51e584940d5b141cd1d09d2629d69b595f579`.

The repository wrapper was not used because it always calls its firmware packer after `west build`. Instead, Zephyr's build command was invoked directly with:

- board: `ats3089c_dev_watch`;
- application: the reference `hello_world`;
- vendor Zephyr: 2.7.0 fork;
- west: 1.5.0;
- ARM toolchain: official Zephyr SDK 0.13.2 standalone ARM toolchain;
- output: Zephyr ELF/BIN/HEX only, outside this repository.

Build result:

| Metric | Result |
| --- | ---: |
| Ninja targets | 802 / 802 |
| Flash region | 1,222,568 bytes / 2,780 KiB (42.95%) |
| SRAM region | 311 KiB / 911 KiB (34.14%) |
| PSRAM region | 5,796,192 bytes / 8 MiB (69.10%) |
| ELF SHA-256 | `4f4356b4e2bc6dcadd4429531d8b646954031f4b2bc9b072f1f600131a64ec0b` |

These numbers describe the feature-heavy reference hello-world configuration, not our simulator application and not the retail watch.

The current Zephyr SDK 1.0.1 reached compilation but failed because its newlib headers are incompatible with the old vendor Zephyr tree. The historical 0.13.2 ARM toolchain compiled successfully. Its optional GDB binary reports a missing `libpython3.8.so.1.0` on Ubuntu 24.04, but GDB was not needed for compilation.

## Vendor constraints

- The reference documentation says application firmware can be built on Linux or Windows.
- It says SoC configuration, blob installation, filesystem resources, and initial download use a Windows-only configuration tool.
- Its wrapper performs resource generation, boot-image assembly, application packing, and firmware construction after compiling.
- Its board directory includes bootloader/recovery/radio/storage binaries. Those files were not executed or imported.
- Its ATS3089C board configuration selects Cortex-M33. Community CMF research claims Cortex-M4 for the retail watch; this conflict remains unresolved.

## Safe reproduction

After independently cloning the pinned source and installing the 0.13.2 ARM toolchain, run `scripts/check-vendor-reference.sh`. The script directly invokes `west build`; it contains no pack, flash, debug, DFU, OTA, or device command.
