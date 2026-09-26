# Offline rollback preparation

This directory prepares evidence for a future recovery procedure. It does not establish that the retail watch is safe to flash.

## Safety properties

- The toolkit operates only on ordinary files supplied by path.
- It contains no USB, serial, BLE, SWD, J-Link, DFU, ADFU, flash, pack, sign, or OTA-write command.
- It does not execute firmware or vendor binaries.
- Input artifacts are opened read-only.
- Generated JSON is written atomically to an explicit output path.
- Symlinks inside an inventory tree are rejected.
- Firmware binaries and personal device backups are ignored by Git.

## Create and verify an artifact inventory

Keep artifacts outside the Git repository when possible:

```bash
mkdir -p /home/rana/cmf-recovery-artifacts/stock-1.0.0.73
python3 tools/recovery_tool.py inventory \
  --input /home/rana/cmf-recovery-artifacts/stock-1.0.0.73 \
  --output /home/rana/cmf-recovery-artifacts/stock-1.0.0.73.inventory.json

python3 tools/recovery_tool.py verify \
  --input /home/rana/cmf-recovery-artifacts/stock-1.0.0.73 \
  --manifest /home/rana/cmf-recovery-artifacts/stock-1.0.0.73.inventory.json
```

Copy the manifest to at least two independent storage locations. A manifest proves later files match the inventoried bytes; it does not prove the source was genuine or that an image is safe to restore.

## Compare two future readbacks

When a safe read-only acquisition method exists, take two independent reads and require exact identity:

```bash
python3 tools/recovery_tool.py compare \
  --first read-1.bin \
  --second read-2.bin \
  --output comparison.json
```

The command exits non-zero when size or SHA-256 differs.

## Inspect OTA metadata

```bash
python3 tools/recovery_tool.py ota-info \
  --input info.xml \
  --output ota-summary.json
```

This reads metadata only. Partition names and sizes in an OTA package are not equivalent to physical flash offsets and must not be used as a flash map.

## Documents

- `device-baseline.md`: information to record manually before hardware access.
- `provenance-template.json`: provenance record for each acquired artifact set.
- `go-no-go.md`: mandatory recovery gate before the first device write.

Run `./scripts/test-recovery-toolkit.sh` to test the toolkit with generated temporary fixtures.
