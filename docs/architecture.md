# Architecture

The watch runtime owns rendering, navigation, input, animations, cache, clock, and connection state. Host communication supplies state and commands through a transport abstraction; it never supplies pixels or blocks ordinary navigation.

The desktop simulator uses the same Zephyr and LVGL application layer intended for later hardware integration. Hardware-specific display, input, storage, transport, and power functions remain behind platform boundaries.

## Watch runtime modules

| Module | Responsibility |
| --- | --- |
| `runtime` | Boot, dependency initialization, event loop, and screen lifecycle coordination |
| `navigation` | Bounded local screen stack with push, pop, replace, and current-screen operations |
| `renderer` | LVGL object creation and state-to-widget rendering |
| `input` | Normalized simulated hardware-button events; SDL pointer input remains supplied by Zephyr |
| `state` | Central watch data and active-screen model |
| `animation` | Direction-aware screen transition policy |
| `cache` | Bounded-cache metadata contract; persistent storage is deferred |
| `transport` | Connection boundary; intentionally offline until the TCP phase |

`main.c` only enters the runtime. Renderer callbacks emit semantic actions, the runtime applies them to navigation/state, and the renderer receives the resulting state. No navigation path performs a host round trip.

## Application model

Applications are registered as compiled native descriptors. The registry owns stable IDs, names, screen identities, offline copy, and availability metadata. Runtime services own navigation, rendering, state, and input, preventing individual apps from coupling themselves to SDL or future CMF hardware drivers.

The initial registry contains Home, Notifications, Music, Assistant, and Settings. All five open while the transport abstraction reports disconnected.
