# CMF Watch Pro 2 board skeleton

This directory is an evidence-gated board-port scaffold, not a flashable Zephyr board.

Files use the `.template` suffix so Zephyr cannot accidentally discover or build this target. Promote them to real board files only after the corresponding values are independently verified and a recovery procedure is proven on sacrificial hardware.

Known direction:

- candidate SoC family: Actions Technology ATS3089C / vendor `leopard` family;
- physical display: round 466 x 466;
- application UI: LVGL;
- likely storage split: code plus external resource/font/data partitions.

Still `UNKNOWN`:

- CPU core identity for the exact retail silicon;
- oscillator and clock tree;
- SRAM/PSRAM/flash sizes and address map;
- display panel controller, MIPI timing, reset/backlight GPIOs;
- touch controller and interrupt/reset GPIOs;
- button/crown GPIOs and polarity;
- battery gauge/charger and ADC wiring;
- sensor models, buses, addresses, and interrupts;
- Bluetooth controller integration and firmware;
- partition offsets, boot chain, signing, rollback, and recovery behavior.

Do not copy the Actions evaluation-board values into these templates: the EVB proves SDK support for the SoC family, not the retail watch wiring.
