# Architecture

The watch runtime owns rendering, navigation, input, animations, cache, clock, and connection state. Host communication supplies state and commands through a transport abstraction; it never supplies pixels or blocks ordinary navigation.

The desktop simulator uses the same Zephyr and LVGL application layer intended for later hardware integration. Hardware-specific display, input, storage, transport, and power functions remain behind platform boundaries.

