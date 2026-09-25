# Hardware evidence register

This register separates retail-watch facts from evaluation-board and community evidence. `CONFIRMED` means supported by an official retail specification or an artifact identified as belonging to the retail model. `LIKELY` means multiple sources align but the exact board has not been independently inspected. `UNKNOWN` is the default.

| Property | Status | Evidence and constraint |
| --- | --- | --- |
| Product display shape | CONFIRMED | Official CMF product material specifies a circular display. |
| Product display resolution | CONFIRMED | Official CMF product material specifies 466 x 466. |
| Stock image board name | CONFIRMED for the examined image | Community dump metadata contains `jx402_01_3089c`; provenance is community-supplied. |
| SoC | LIKELY | Firmware strings/metadata and independent BLE research identify ATS3089C; no official retail schematic or readable chip marking has been inspected. |
| CPU core | UNKNOWN | The Actions ATS3089C reference selects Cortex-M33; community BLE research calls the retail device Cortex-M4. Do not choose one yet. |
| RTOS | LIKELY | Firmware strings research reports Zephyr, and the Actions ATS3089C reference is a Zephyr 2.7 fork. |
| UI library | LIKELY | Firmware strings research reports LVGL, and the Actions reference integrates LVGL. |
| Retail display interface/panel | UNKNOWN | The reference EVB uses a 466 x 466 MIPI-DSI panel, but EVB wiring is not retail-board evidence. |
| Touch controller and wiring | UNKNOWN | No retail-board evidence. |
| Internal/external memory | UNKNOWN | Evaluation-board memory values are not copied to the retail target. |
| Buttons/crown | UNKNOWN | GPIOs, polarity, debounce, wake behavior, and whether the rotating crown is electrically available are unverified. |
| Battery/charger/gauge | UNKNOWN | No verified device identity, bus, or calibration data. |
| Sensors | UNKNOWN | No verified models, addresses, interrupts, orientation, or calibration. |
| BLE transport | LIKELY protocol, UNKNOWN integration | Community work documents a confidence-labelled GATT protocol. Our firmware-side controller/driver path is unverified and no BLE operation has been performed. |
| Storage/partitions | UNKNOWN | Stock metadata shows named resource/font/data partitions, but offsets, flash topology, boot partitions, and safe write rules are not established. |
| Boot/signing/recovery | UNKNOWN | No proven readback, signature model, rollback, unbrick, or stock restore process exists. |

The community BLE document's `466x360` statement conflicts with the official 466 x 466 product resolution and is not used by this project.

## Safety gate

The board scaffold is intentionally non-buildable. Before any physical firmware operation, all of the following are required:

- repeatable stock-firmware readback or a manufacturer-verified full restore image;
- understood boot stages, partition table, image validation, and rollback behavior;
- a recovery/DFU path proven on sacrificial hardware;
- verified power, clock, memory, display, and input mappings;
- an application-only image that cannot overwrite boot/recovery/calibration data;
- explicit user approval for the exact device operation.

No physical-watch, BLE, firmware, bootloader, NAND/SPI, DFU, or OTA operation has been performed.
