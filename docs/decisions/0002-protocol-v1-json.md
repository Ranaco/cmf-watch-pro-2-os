# ADR 0002: Versioned JSON Protocol over an Abstract Transport

## Status

Accepted — 2026-09-24.

## Decision

Protocol v1 uses a strict semantic envelope and newline-delimited JSON during simulator development. Message semantics do not depend on TCP, BLE, UART, or WebSocket. Rendering instructions and framebuffer data are prohibited.

## Consequences

- Packets are human-readable during early debugging.
- Strict schema and semantic tests detect incompatible changes.
- JSON overhead is accepted temporarily.
- CBOR or a compact binary codec can replace JSON without changing application behavior.
