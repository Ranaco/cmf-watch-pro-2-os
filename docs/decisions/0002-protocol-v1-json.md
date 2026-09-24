# ADR 0002: Versioned JSON Protocol over an Abstract Transport

## Status

Accepted — 2026-09-24.

## Decision

Protocol v1 uses a strict semantic envelope and newline-delimited JSON during simulator development. Message semantics do not depend on TCP, BLE, UART, or WebSocket. Rendering instructions and framebuffer data are prohibited.

The envelope version is fixed during bootstrap. Peers reject unsupported protocol versions and unknown message types, but ignore unknown additive fields in otherwise recognized envelopes and payloads. A host-generated `session_id` scopes message identifiers for each connection session.

The host owns synchronized service data; the watch owns local UI, navigation, and device settings. State updates use registered dot paths with unambiguous segments. `state_set` replaces the value at a registered path, while protocol v1 `state_patch` changes registered leaf paths only. Revisions apply only when they are exactly the current revision plus one: stale updates are ignored and gaps trigger a fresh synchronization.

During synchronization, incremental updates are buffered, the validated snapshot is installed atomically, and only contiguous buffered revisions are applied before entering the synchronized state. Commands always produce a `command_result` with a stable status and optional error code.

TCP framing is one UTF-8 JSON object per LF-terminated frame. A receiver accepts an optional CR before LF, rejects invalid UTF-8, empty frames, embedded raw newlines, and frames over the negotiated limit, and never treats a partial frame as a message.

## Consequences

- Packets are human-readable during early debugging.
- Strict schema and semantic tests detect incompatible changes.
- Shared conformance fixtures define codec behavior across the C/watch and Rust/host implementations.
- JSON overhead is accepted temporarily.
- CBOR or a compact binary codec can replace JSON without changing application behavior.
