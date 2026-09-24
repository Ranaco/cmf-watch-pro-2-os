# Development Environment

Verified on 2026-09-24 in WSL2 Ubuntu 24.04.4 LTS.

| Tool | Version |
| --- | --- |
| Git | 2.43.0 |
| Python | 3.12.3 |
| CMake | 3.28.3 |
| Ninja | 1.11.1 |
| Device Tree Compiler | 1.7.0 |
| GCC / G++ | 13.3.0 |
| Clang | 18.1.3 |
| SDL2 | 2.30.0 |
| Rust compiler | 1.98.1 |
| Cargo | 1.98.1 |

## Verification

- `gcc` compiled and ran a minimal C program.
- `g++` compiled and ran a minimal C++ program.
- SDL2 was discovered through `pkg-config`.
- Rust was installed through `rustup` using the minimal stable profile.

## Shell setup

Rust is installed under `~/.cargo`. New Bash sessions load it through:

```bash
. "$HOME/.cargo/env"
```

## Scope

This environment is for the desktop simulator and later reference-target builds. No physical watch operation, BLE communication, firmware acquisition, flashing, or bootloader work has occurred.
