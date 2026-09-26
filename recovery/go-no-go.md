# First-write recovery gate

The default decision is **NO-GO**. Every mandatory item must have dated evidence before changing it.

## Mandatory evidence

- [ ] Exact retail model and currently installed firmware are recorded.
- [ ] Stock artifacts have provenance records and SHA-256 inventories.
- [ ] Two independent full readbacks from the same donor device are byte-identical.
- [ ] Internal and external storage geometry is independently verified.
- [ ] Bootloader, recovery, application, resources, radio data, NVRAM, calibration, and user-data regions are mapped.
- [ ] OTP/fuse/security state is understood without attempting to modify it.
- [ ] Image authentication, signing, version counters, and anti-rollback behavior are understood.
- [ ] A recovery/DFU entry method is repeatable after application corruption.
- [ ] Recovery does not depend on the normal application, touch screen, or Bluetooth stack.
- [ ] The original donor backup has been restored and verified on that donor.
- [ ] Display, touch, controls, charging, sensors, audio/vibration, Bluetooth, official pairing, reboot, and factory reset pass after restoration.
- [ ] The proposed first custom image changes only a verified application slot.
- [ ] A written command plan identifies every byte range that can be written.
- [ ] A second person or independent review has checked the restore and write plans.
- [ ] The user has explicitly approved the exact device and exact write operation.

## Automatic NO-GO conditions

- Only an OTA package or community dump is available, not a device-specific full backup.
- The restore path has been inferred from an evaluation board.
- Any partition offset, pin, voltage, storage type, image signature, or recovery sequence is guessed.
- Backup reads disagree.
- The only recovery path requires the running application or a working Bluetooth connection.
- The operation would erase bootloader, recovery, radio, calibration, OTP, or unknown regions.
- The primary watch would be the first restoration test target.

Meeting this checklist reduces risk; it cannot guarantee recovery from electrical damage, locked security state, ROM incompatibility, or incorrect hardware assumptions.
