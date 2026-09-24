# ADR 0001: Round 466×466 Display Geometry

## Status

Accepted — 2026-09-24.

## Context

The original project brief described the CMF Watch Pro 2 display as 466×360. The physical device is round, and CMF's official support specification lists a 466×466 resolution, 1.32-inch size, and 353 PPI.

Source: <https://support.cmf.tech/hc/en-us/articles/26428710541329-Watch-Pro-2-display-specifications>

## Decision

The simulator display is 466×466. The UI uses a 438×438 circular root surface, leaving a 14-pixel simulator bezel on every side, and keeps important content within a conservative circular safe area. Black square-window corners represent pixels outside the physical round panel.

## Consequences

- Layouts must account for reduced usable width near the top and bottom of the circle.
- Simulator geometry no longer follows the incorrect 466×360 value in the original brief.
- Hardware and screenshot comparisons use 466×466 as the canonical display resolution.
