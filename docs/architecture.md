# Architecture

The watch runtime owns rendering, navigation, input, animations, cache, clock, and connection state. Host communication supplies state and commands through a transport abstraction; it never supplies pixels or blocks ordinary navigation.

The desktop simulator uses the same Zephyr and LVGL application layer intended for later hardware integration. Hardware-specific display, input, storage, transport, and power functions remain behind platform boundaries.

## Watch runtime modules

| Module | Responsibility |
| --- | --- |
| `runtime` | Boot, dependency initialization, event loop, and screen lifecycle coordination |
| `navigation` | Local five-page carousel plus bounded stack primitives |
| `renderer` | Sparse round-face LVGL rendering and full-face swipe event capture |
| `input` | Gesture classification and normalized simulated hardware-button events; SDL pointer input remains supplied by Zephyr |
| `state` | Central watch data, active-screen model, and strict host synchronization service |
| `animation` | Direction-aware screen transition policy |
| `cache` | Bounded-cache metadata contract; persistent storage is deferred |
| `transport` | Framed connection boundary with a simulator-only localhost TCP backend |

`main.c` only enters the runtime. Renderer callbacks emit next/previous actions, the runtime applies them to navigation/state, and the renderer receives the resulting state. Horizontal gestures require a decisive 50-pixel movement and reject mostly vertical drags. No navigation path performs a host round trip.

## Application model

Applications are registered as compiled native descriptors. The registry owns stable IDs, names, screen identities, offline copy, and availability metadata. Runtime services own navigation, rendering, state, and input, preventing individual apps from coupling themselves to SDL or future CMF hardware drivers.

The initial registry contains Home, Notifications, Music, Assistant, and Settings. All five open while the transport abstraction reports disconnected.

## Layer invariants

- Transport moves framed bytes and does not mutate state.
- Protocol codecs parse and emit semantic messages and do not touch LVGL.
- The host publishes domain state and never controls pixels or widget coordinates.
- The renderer consumes watch state and never calls host services.
- Navigation is local and never requires a connection.
- State ownership is enforced before any mutation reaches the state store.

## Simulator host path

The Rust host listens on localhost TCP port 4660. The Zephyr `native_sim` build uses native offloaded sockets to connect without TAP setup, then passes bounded frames into the C protocol codec. The codec emits semantic messages only; the transport never writes `WatchState` and the renderer never observes socket state directly.

Decoded state messages pass through the watch synchronization service. It atomically installs snapshots, buffers up to 16 pre-snapshot mutations, replays only contiguous revisions, rejects unregistered paths, and requests a fresh snapshot when it detects a revision gap. Only the resulting `WatchState` reaches the renderer.
