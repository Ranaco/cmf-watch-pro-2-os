# CMF Watch Pro 2 Thin-Client OS

## Mission

Build a custom lightweight operating environment for the CMF Watch Pro 2.

The watch must NOT behave like a remote framebuffer.

Instead:

* The watch renders its UI locally with LVGL.
* The watch owns navigation, animations, gestures, local caching, clock, connection state and basic interaction.
* A remote PC owns heavy computation, APIs, AI, application logic and large datasets.
* The PC synchronizes state/data with the watch.
* Normal navigation must remain responsive even when the PC is temporarily unreachable.
* The same application/runtime architecture must work first in a desktop simulator and later on the actual ATS3089C hardware.

The development PC is:

* Windows
* Ryzen 5
* 32 GB RAM
* RTX 3060-class GPU
* 1 TB storage

Use Windows + WSL2 Ubuntu 24.04 as the primary development environment.

Do NOT flash the physical CMF Watch Pro 2 until the project reaches the hardware-specific phases below and all safety gates have been met.

---

# Core Architecture

Target architecture:

```text
                    HOST PC
             ┌─────────────────┐
             │ Host Runtime    │
             │                 │
             │ AI              │
             │ APIs            │
             │ Notifications   │
             │ Application     │
             │ logic           │
             │ Data services   │
             └────────┬────────┘
                      │
               State / Commands
                      │
              Transport abstraction
               TCP first, BLE later
                      │
                      ▼
             ┌───────────────────┐
             │ Watch Runtime     │
             │                   │
             │ App registry      │
             │ UI scene state    │
             │ Navigation        │
             │ Input             │
             │ Animations        │
             │ Cache             │
             │ Protocol client   │
             │ LVGL Renderer     │
             └─────────┬─────────┘
                       │
                       ▼
                    Display
```

Do not make navigation depend on host round trips.

Example:

```text
Swipe
  ↓
local navigation
  ↓
screen immediately appears
  ↓
background refresh request sent to PC
  ↓
new data updates visible widgets
```

---

# Technology Decisions

Use:

```text
Watch firmware/runtime:
C
Zephyr
LVGL

Host runtime:
Rust

Simulator:
Linux/WSL
LVGL SDL simulator

Initial transport:
TCP localhost

Hardware transport:
BLE

Initial message encoding:
JSON for debugging

Later message encoding:
CBOR or compact binary representation

Build:
CMake
west
Ninja
Cargo

Primary development:
WSL2 Ubuntu 24.04

Windows-native tools where necessary:
BLE utilities
J-Link
Wireshark
vendor flashing tools
USB utilities
```

Create abstractions so JSON/TCP can later be replaced with CBOR/BLE without changing application logic.

---

# Repository

Create:

```text
cmf-watch-os/
│
├── README.md
├── docs/
│   ├── architecture.md
│   ├── protocol.md
│   ├── hardware.md
│   ├── development.md
│   ├── reverse-engineering.md
│   └── decisions/
│
├── watch/
│   ├── CMakeLists.txt
│   ├── prj.conf
│   ├── src/
│   │   ├── main.c
│   │   │
│   │   ├── runtime/
│   │   ├── navigation/
│   │   ├── protocol/
│   │   ├── state/
│   │   ├── cache/
│   │   ├── input/
│   │   ├── animation/
│   │   ├── transport/
│   │   └── renderer/
│   │
│   ├── apps/
│   │   ├── home/
│   │   ├── notifications/
│   │   ├── assistant/
│   │   ├── music/
│   │   └── settings/
│   │
│   └── boards/
│       ├── simulator/
│       └── cmf_watch_pro_2/
│
├── host/
│   ├── Cargo.toml
│   └── src/
│       ├── main.rs
│       ├── device/
│       ├── protocol/
│       ├── state/
│       ├── transport/
│       ├── services/
│       └── apps/
│
├── protocol/
│   ├── schema/
│   ├── examples/
│   └── README.md
│
├── tools/
│   ├── firmware/
│   ├── ble/
│   └── scripts/
│
├── research/
│   ├── firmware/
│   ├── ble/
│   └── hardware/
│
└── scripts/
    ├── bootstrap.sh
    ├── build-watch.sh
    ├── run-simulator.sh
    ├── build-host.sh
    └── run-dev.sh
```

Do not unnecessarily couple the simulator implementation to the CMF hardware implementation.

---

# Agent Working Rules

Codex must follow these rules throughout the project.

1. Execute work in phases.
2. Do not move to the next phase until acceptance criteria pass.
3. Commit working milestones.
4. Keep documentation synchronized with implementation.
5. Never silently ignore errors.
6. Prefer reproducible scripts over undocumented manual commands.
7. Do not flash the CMF watch during early development.
8. Do not perform NAND writes or bootloader modifications without an explicit later approval.
9. Treat unknown hardware details as unknown rather than inventing values.
10. Keep simulator, runtime and protocol transport-independent.
11. Add tests when implementing protocol/state logic.
12. Keep a `docs/development-status.md` file containing:

    * completed tasks
    * current task
    * blockers
    * discoveries
    * next task

Commit after each major milestone.

---

# PHASE 0 — Inspect Development Machine

First determine the existing environment.

From Windows inspect:

```powershell
wsl --status
wsl --list --verbose
git --version
code --version
```

Determine whether Ubuntu 24.04 already exists.

If WSL is missing, install:

```powershell
wsl --install -d Ubuntu-24.04
```

Do not delete or modify existing WSL distributions.

Inside Ubuntu inspect:

```bash
lsb_release -a
uname -a
git --version
python3 --version
cmake --version
ninja --version
gcc --version
```

Create a status report before changing anything.

Acceptance:

```text
WSL2 operational
Ubuntu 24.04 operational
network access operational
Git operational
```

---

# PHASE 1 — Bootstrap Linux Development Environment

Update Ubuntu:

```bash
sudo apt update
sudo apt upgrade -y
```

Install required development dependencies.

At minimum:

```bash
sudo apt install --no-install-recommends \
git \
cmake \
ninja-build \
gperf \
ccache \
dfu-util \
device-tree-compiler \
wget \
python3-dev \
python3-venv \
python3-tk \
xz-utils \
file \
make \
gcc \
gcc-multilib \
g++-multilib \
libsdl2-dev \
libmagic1 \
pkg-config \
clang \
lld \
libssl-dev
```

Verify:

```bash
cmake --version
python3 --version
dtc --version
ninja --version
```

Install Rust with rustup if absent.

Verify:

```bash
rustc --version
cargo --version
```

Create:

```text
docs/environment.md
```

containing detected versions.

Acceptance:

```text
C/C++ compiler working
Rust working
Python working
CMake working
Ninja working
SDL2 available
```

---

# PHASE 2 — Install Zephyr Properly

Use an isolated Zephyr workspace.

Prefer:

```text
~/zephyrproject
```

Create Python environment:

```bash
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate
pip install --upgrade pip
pip install west
```

Initialize Zephyr:

```bash
west init -m https://github.com/zephyrproject-rtos/zephyr ~/zephyrproject
cd ~/zephyrproject
west update
west packages pip --install
west zephyr-export
```

Install SDK:

```bash
cd ~/zephyrproject/zephyr
west sdk install
```

Do not start CMF-specific work yet.

First compile standard Zephyr applications.

Build Hello World against a supported native/simulation target.

Record exact successful commands in:

```text
docs/zephyr-setup.md
```

Acceptance:

```text
west works
Zephyr SDK installed
Zephyr sample builds successfully
Zephyr sample executes successfully
```

Commit:

```text
chore: establish Zephyr development environment
```

---

# PHASE 3 — Establish LVGL Desktop Simulator

We need the watch UI to be developed without physical hardware.

Display target:

```text
width: 466
height: 360
```

Create an SDL-backed LVGL simulator.

The simulator must open a window exactly representing the CMF Watch Pro 2 screen area.

Initial screen:

```text
┌─────────────────────────┐
│ 21:45               72% │
│                         │
│        Thursday         │
│      September 24       │
│                         │
│         28°C            │
│                         │
│      7,421 steps        │
│                         │
└─────────────────────────┘
```

Implement:

* 466×360 resolution
* mouse -> touch mapping
* keyboard shortcuts for hardware buttons
* optional mouse wheel -> crown mapping
* 30/60 FPS rendering
* correct screen transitions

No networking yet.

Acceptance:

```text
simulator builds with one command
simulator launches
home screen renders
touch/mouse input works
screen animations work
```

Provide:

```bash
./scripts/run-simulator.sh
```

Commit:

```text
feat: add 466x360 LVGL watch simulator
```

---

# PHASE 4 — Build Watch Runtime Architecture

Do not hard-code the whole UI inside `main.c`.

Implement layers.

## Runtime

Responsible for:

```text
boot
app lifecycle
screen lifecycle
system events
connection state
```

## Renderer

Responsible for:

```text
LVGL object creation
widget updates
styles
animations
asset loading
```

## Navigation

Responsible for:

```text
push
pop
replace
swipe navigation
modal overlays
home gesture
```

## Input

Normalize:

```text
tap
long press
swipe left
swipe right
swipe up
swipe down
button press
button long press
crown rotate
```

## State Store

Implement a small central state model.

Example:

```text
WatchState
├── connection
├── battery
├── clock
├── active_app
├── navigation
├── notifications
├── music
└── assistant
```

UI widgets subscribe to relevant state.

Acceptance:

Navigation between several dummy screens must require no PC.

Commit:

```text
feat: establish local watch runtime and navigation
```

---

# PHASE 5 — Define Watch Application Model

Applications are lightweight frontend packages.

An app contains:

```text
metadata
screen descriptions
local state
event handlers
assets
navigation rules
```

It should NOT contain heavyweight backend functionality.

Start with:

```text
Home
Notifications
Music
Assistant
Settings
```

Example behavior:

```text
HOME

swipe right
    ↓

NOTIFICATIONS

swipe left
    ↓

HOME

swipe left
    ↓

ASSISTANT
```

All those transitions happen locally.

The PC is NOT consulted before navigating.

Acceptance:

All five apps can be opened offline.

---

# PHASE 6 — Define Host ↔ Watch Protocol

Create a protocol specification before implementing network communication.

Separate messages into:

## State updates

```text
STATE_SET
STATE_PATCH
LIST_INSERT
LIST_REMOVE
LIST_UPDATE
```

## Commands

```text
REFRESH
APP_OPEN
SHOW_TOAST
SHOW_DIALOG
CACHE_INVALIDATE
```

## Watch events

```text
APP_OPENED
ACTION
BUTTON
GESTURE
INPUT
REFRESH_REQUEST
```

## Connection

```text
HELLO
CAPABILITIES
PING
PONG
SYNC_REQUEST
SYNC_RESPONSE
```

Every packet must have:

```text
version
message type
request/event id
payload
```

Example during development:

```json
{
  "version": 1,
  "type": "state_patch",
  "id": 82,
  "payload": {
    "path": "weather.temperature",
    "value": 28
  }
}
```

Document everything in:

```text
docs/protocol.md
```

Add protocol versioning from the beginning.

---

# PHASE 7 — Implement Transport Abstraction

The runtime must not know whether communication happens over:

```text
TCP
BLE
UART
WebSocket
```

Define something conceptually equivalent to:

```c
transport_send(...)
transport_receive(...)
transport_connected(...)
transport_disconnect(...)
```

The host has an equivalent abstraction.

First implementation:

```text
TCP localhost
```

Do not implement BLE yet.

Why:

The simulator and host can be developed/debugged with ordinary networking before introducing embedded BLE problems.

---

# PHASE 8 — Build Rust Host Runtime

Create the Rust daemon in:

```text
host/
```

It manages connected watches.

Implement:

```text
DeviceManager
StateStore
ProtocolCodec
TcpTransport
AppServices
```

Host should listen on localhost.

Example:

```text
127.0.0.1:7331
```

When simulator connects:

```text
SIMULATOR
     ↓
HELLO
     ↓
HOST
     ↓
SYNC_RESPONSE
```

Host logs all traffic in development mode.

Do not mix UI rendering code into the host.

---

# PHASE 9 — First End-to-End Demo

Target:

Watch simulator shows:

```text
21:45
28°C
7,421 steps
```

The PC host owns:

```text
temperature
step count
notification list
music metadata
```

Changing host state automatically updates the simulator.

Navigation remains local.

Test latency artificially.

Add:

```text
100ms
300ms
1000ms
offline
```

simulated host latency.

The UI should remain usable under all cases.

Acceptance test:

While disconnected from PC:

```text
swipe screens
open settings
scroll cached notifications
return home
view clock
```

must still work.

After PC reconnects:

```text
watch automatically synchronizes
```

Commit:

```text
feat: complete host-to-watch simulator pipeline
```

This is the first major project milestone.

---

# PHASE 10 — Implement Local Cache

Create bounded watch-side storage abstraction.

Cache:

```text
last notifications
last weather data
music metadata
UI assets
application state
host connection metadata
```

Do NOT cache sensitive information unnecessarily.

Every cached object should support:

```text
value
timestamp
version
stale flag
```

UI may indicate stale data where appropriate.

Example:

```text
Weather
28°C

Updated 42m ago
```

---

# PHASE 11 — Optimistic Actions

Actions must feel immediate.

Example music Play button:

```text
tap
 ↓
watch immediately changes icon
 ↓
ACTION(play) sent to host
 ↓
host executes action
 ↓
host sends confirmation
```

Failure:

```text
host returns failure
 ↓
watch restores previous state
 ↓
small error toast
```

Use this for:

```text
music controls
task completion
smart-home actions
notification dismissal
```

---

# PHASE 12 — App SDK / Declarative UI Exploration

After the runtime works, design a lightweight app manifest.

Example:

```json
{
  "id": "music",
  "name": "Music",
  "version": 1,
  "entry": "now_playing"
}
```

Do NOT build a complicated JavaScript interpreter.

The first app system should compile UI/application definitions into native structures.

Long-term we may support declarative application bundles.

Do not over-engineer this phase.

---

# PHASE 13 — Automated Tests

Add:

## Host

Rust unit/integration tests for:

```text
protocol serialization
state patches
reconnect
sequence ordering
invalid packets
version compatibility
```

## Watch

Tests for:

```text
navigation
state reducers
protocol parsing
cache behavior
offline behavior
```

Add a soak test:

```text
host + simulator run for one hour
state updates continuously
connection periodically drops/reconnects
```

Detect:

```text
memory leaks
crashes
protocol desync
navigation corruption
```

---

# PHASE 14 — Performance Budget

Establish measurable budgets.

Target:

```text
local navigation response: < 50 ms

normal screen animation:
30+ FPS minimum
prefer 60 FPS where practical

remote state patch:
small enough for BLE operation

normal screen opening:
no host round trip required

offline startup:
must succeed

reconnect:
automatic
```

Instrument:

```text
render time
frame time
message size
message frequency
memory usage
heap high-water mark
```

---

# PHASE 15 — Begin CMF Watch Pro 2 Research

ONLY start this phase after simulator milestone is stable.

Research currently known:

```text
Device: CMF Watch Pro 2
MCU: Actions ATS3089C
RTOS: Zephyr
UI: LVGL
Display: 466×360
```

Create:

```text
docs/hardware.md
research/hardware/
research/firmware/
research/ble/
```

Do not treat community reverse-engineering claims as guaranteed unless independently verified.

Track confidence:

```text
CONFIRMED
LIKELY
UNKNOWN
```

---

# PHASE 16 — Clone Reference Projects

For research only, clone/reference:

```text
lvgl/lv_port_actions_technology

whatotter/cmf-watch-firmware

joshuapassos/CMF-Watch-Pro-2-BLE-Protocol
```

Keep these outside production source or under:

```text
research/reference/
```

Do not blindly copy code.

Determine:

```text
ATS3089C build configuration
vendor SDK requirements
Zephyr version
LVGL integration
memory layout
flash tooling
UART usage
BLE characteristics
OTA structure
```

Produce:

```text
docs/actions-platform.md
```

---

# PHASE 17 — Compile ATS3089C Reference Firmware

Goal:

Compile an existing Actions Technology ATS3089C development-watch target.

Do NOT flash the CMF watch.

Only prove:

```text
toolchain works
vendor dependencies work
ATS3089C binary can be built
```

Capture:

```text
binary size
map file
sections
toolchain versions
board configuration
```

Acceptance:

An ATS3089C-targeted firmware artifact builds successfully.

---

# PHASE 18 — Introduce Hardware Abstraction

Separate runtime from hardware.

Define interfaces:

```text
Display
Touch
Button
Crown
Battery
RTC
Storage
BLE
Sensors
Power
```

Simulator provides fake implementations.

CMF board eventually provides real implementations.

No app should access hardware registers directly.

---

# PHASE 19 — CMF Board Definition Skeleton

Create:

```text
boards/cmf_watch_pro_2/
```

But do not invent pin assignments.

Unknowns should look like:

```text
TODO_DISPLAY_CONTROLLER
TODO_TOUCH_CONTROLLER
TODO_TOUCH_IRQ_GPIO
TODO_CROWN_GPIO
```

not random values.

Populate only confirmed information.

Track every discovery and its source.

---

# PHASE 20 — BLE Protocol Experimentation

Now investigate communication with the physical watch.

First operations must be read-only/non-destructive.

Build tooling under:

```text
tools/ble/
```

Capabilities:

```text
scan
identify Watch Pro 2
enumerate services
subscribe to notifications
log packets
decode known packet structures
```

Store captures with timestamps.

Do NOT send firmware.

Do NOT write flash.

Do NOT trigger undocumented destructive commands.

---

# PHASE 21 — Firmware Acquisition and Analysis

Obtain official/community firmware through legitimate means.

Store originals as immutable files.

For each image record:

```text
SHA256
firmware version
source
date
size
```

Never modify the original.

Create derived working copies.

Analyze:

```text
headers
info.xml
partition structure
compression
filesystem
strings
Zephyr metadata
LVGL assets
UART references
bootloader references
OTA references
```

Document findings.

---

# PHASE 22 — Determine Boot Chain

Before any flashing attempt establish:

```text
ROM boot
    ↓
bootloader
    ↓
firmware verification?
    ↓
partition selection
    ↓
Zephyr image
```

Questions:

1. Is firmware cryptographically signed?
2. Is there merely a checksum?
3. Is rollback protection present?
4. Is there A/B firmware?
5. Is there a recovery partition?
6. Where is the bootloader stored?
7. Can firmware update touch the bootloader?
8. What triggers recovery/DFU?
9. Is J-Link/SWD accessible?
10. Is UART accessible?
11. Is there a ROM-level recovery method?

No custom firmware flashing until these are reasonably answered.

---

# PHASE 23 — Physical Hardware Safety Gate

Before opening/flashing the user's primary watch, stop and generate:

```text
docs/hardware-risk-review.md
```

It must contain:

```text
confirmed recovery path
known bootloader behavior
known firmware backup strategy
known flash layout
known programming interface
risk of permanent brick
rollback strategy
required hardware
```

STOP EXECUTION HERE.

Require explicit human authorization before doing:

```text
opening watch
soldering
SWD write
SPI/NAND write
custom OTA
bootloader modification
```

---

# PHASE 24 — Future Hardware Bring-Up

Only after explicit authorization.

Target order:

```text
UART logs
 ↓
read-only debugger access
 ↓
flash backup
 ↓
display initialization
 ↓
touch
 ↓
button/crown
 ↓
storage
 ↓
BLE
 ↓
power management
 ↓
battery
 ↓
sensors
```

First custom firmware should do almost nothing:

```text
boot
initialize display
draw solid background
display:

"HELLO CMF"

respond to one button
```

Nothing more.

---

# PHASE 25 — Port Existing Runtime

Once hardware drivers are proven:

Move the already-tested simulator runtime to:

```text
cmf_watch_pro_2
```

The application/runtime/protocol layers should require minimal modification because hardware dependencies were abstracted earlier.

Target:

```text
Watch boots
 ↓
Home renders
 ↓
local gestures work
 ↓
BLE connects
 ↓
host syncs
 ↓
data appears
```

---

# PHASE 26 — Power Optimization

Only after functionality works.

Optimize:

```text
CPU sleep
display sleep
BLE intervals
render invalidation
timer frequency
sensor polling
cache writes
animation FPS
background work
```

Measure battery rather than guessing.

---

# PHASE 27 — PC Backend Expansion

After watch fundamentals work, add integrations modularly:

```text
Weather
Calendar
Notifications
Music
Home Assistant
AI
PC control
Tasks
Messaging
```

Each host service should produce generic state/events rather than watch-specific rendering instructions.

Correct:

```text
music.title = "..."
music.playing = true
```

Avoid:

```text
draw text at x=42 y=83
```

---

# PHASE 28 — AI Layer

The RTX GPU can later power optional local AI.

Possible architecture:

```text
Watch
 ↓
Assistant event
 ↓
Host daemon
 ↓
AI service
 ↓
result
 ↓
Watch state update
```

Never require AI to navigate ordinary watch UI.

If AI is unavailable, watch remains usable.

Possible later capabilities:

```text
voice commands
summaries
contextual cards
calendar assistance
PC control
smart-home control
coding-agent status
```

Do not implement AI during the initial OS foundation.

---

# Initial Definition of Done

The first development cycle is complete when:

1. Windows + WSL environment is reproducible.
2. Zephyr builds successfully.
3. LVGL simulator runs at 466×360.
4. Watch has Home, Notifications, Music, Assistant and Settings.
5. Swiping/navigation happens entirely locally.
6. Rust host daemon runs.
7. Host and simulator communicate through the transport abstraction.
8. Host state updates watch UI.
9. Watch remains navigable when host disconnects.
10. Reconnection performs state synchronization.
11. Protocol has versioning and tests.
12. ATS3089C reference firmware can be compiled.
13. CMF-specific hardware work remains behind an explicit safety gate.

At that point we have built the actual SYSTEM, not merely a UI demo.

---

# First Execution Session

Start now with ONLY Phases 0–3.

Specifically:

1. inspect Windows/WSL
2. establish Ubuntu environment
3. install Zephyr
4. verify Zephyr with an official sample
5. create repository
6. create documentation skeleton
7. create 466×360 LVGL simulator
8. render initial Home screen
9. make input work
10. commit working result

Do not begin BLE, firmware flashing, bootloader work or CMF hardware modification during the first execution session.

When Phases 0–3 pass, update:

```text
docs/development-status.md
```

with:

```text
environment
installed versions
successful commands
failures encountered
solutions applied
repository state
next milestone
```

Then proceed to Phase 4.
