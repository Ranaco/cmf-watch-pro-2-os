# Zephyr Setup

Verified on 2026-09-24 using WSL2 Ubuntu 24.04.4 LTS.

## Workspace

- Zephyr workspace: `/home/rana/zephyrproject`
- Python environment: `/home/rana/zephyrproject/.venv`
- Zephyr SDK: `/home/rana/zephyr-sdk-1.0.1`
- Project repository: `/home/rana/watch`

## Installed versions

| Component | Version |
| --- | --- |
| Zephyr | 4.4.0-16995-g0d3ec1aeccd3 |
| west | 1.5.0 |
| Zephyr SDK | 1.0.1 |
| Python | 3.12.3 |
| CMake | 3.28.3 |
| Ninja | 1.11.1 |

## Successful setup commands

```bash
python3 -m venv /home/rana/zephyrproject/.venv
. /home/rana/zephyrproject/.venv/bin/activate
python -m pip install --upgrade pip
python -m pip install west
west init -m https://github.com/zephyrproject-rtos/zephyr /home/rana/zephyrproject
cd /home/rana/zephyrproject
west update
west packages pip --install
west zephyr-export
cd /home/rana/zephyrproject/zephyr
west sdk install
```

## Official sample verification

The official Hello World sample was built for the native simulator and executed successfully:

```bash
. /home/rana/zephyrproject/.venv/bin/activate
cd /home/rana/zephyrproject/zephyr
west build -p always -b native_sim samples/hello_world \
  -d /home/rana/zephyrproject/build/hello_world
/home/rana/zephyrproject/build/hello_world/zephyr/zephyr.exe
```

Observed output:

```text
*** Booting Zephyr OS build v4.4.0-16995-g0d3ec1aeccd3 ***
Hello World! native_sim/native
```

## Notes

- Zephyr reported that system ccache 4.9.1 is older than its preferred minimum of 4.12. The build continued without ccache; this is an optimization issue, not a blocker.
- No physical watch, BLE, firmware, bootloader, NAND/SPI, or OTA operation was performed.
