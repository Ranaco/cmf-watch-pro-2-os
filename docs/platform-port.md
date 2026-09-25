# Platform port contract

The application uses standard Zephyr drivers for concrete display, pointer, storage, radio, sensor, and power devices. `watch/src/platform/watch_platform.h` adds a small evidence-gated capability descriptor above those drivers.

The native backend advertises only capabilities exercised by the simulator: a round 466 x 466 display, touch/pointer input, simulated button input, time, cache storage, and process-level power/lifecycle behavior. BLE, crown, battery, and physical sensors remain absent.

A future CMF backend must satisfy this sequence:

1. verify the relevant retail-watch hardware mapping;
2. implement and test the Zephyr device driver or binding;
3. expose it through the application service already responsible for that domain;
4. advertise the capability only after the driver is operational;
5. add an offline/error path before enabling it in the UI.

The scaffold under `boards/cmf_watch_pro_2` uses `.template` filenames deliberately. It cannot be selected as a Zephyr board and contains no guessed addresses or pins.
