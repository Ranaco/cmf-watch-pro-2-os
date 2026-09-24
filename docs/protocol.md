# Host ↔ Watch Protocol

## Goals

The protocol synchronizes semantic state and actions. It never sends pixels, coordinates, or rendering instructions. Local navigation remains available while disconnected.

Version 1 uses newline-delimited JSON over localhost TCP for observability. Encoding and transport are independent: JSON may later become CBOR and TCP may later become BLE without changing message meaning.

## Envelope

```json
{
  "version": 1,
  "type": "state_patch",
  "id": 82,
  "payload": {}
}
```

| Field | Type | Rules |
| --- | --- | --- |
| `version` | unsigned integer | Bootstrap envelope version; exactly `1` |
| `type` | string | Registered snake-case message type |
| `id` | unsigned integer | Sender-local monotonically increasing identifier |
| `payload` | object | Message-specific data |
| `reply_to` | unsigned integer | Optional request ID being answered |
| `timestamp_ms` | unsigned integer | Optional informational sender clock |

Unknown additive envelope and payload fields are ignored unless a message specification explicitly marks them incompatible. Unknown message types, unsupported versions, and malformed packets are rejected without state mutation.

`id` uniqueness is scoped to a runtime session. Every `hello` includes a randomly generated `session_id`, so the stable message identity is `(session_id, id)` and IDs may safely restart at zero after a process restart.

`hello` always uses the lowest bootstrap-compatible envelope format. Its `protocol_versions` array negotiates the semantic version used for subsequent messages. The bootstrap envelope and negotiated semantic protocol may evolve independently.

## State paths and revisions

State paths use dot notation, such as `weather.temperature`. Paths are registered by the protocol or an application schema; individual path segments must not contain `.` and arbitrary user-derived keys are forbidden.

State has a declared owner:

- Host-owned: `weather.*`, `notifications.*`, `music.*`, `calendar.*`, `assistant.*`, `steps`.
- Watch-owned: `system.battery`, `system.connection`, `system.active_app`, `system.clock`, `navigation.*`, `input.*`.

A peer must reject a mutation targeting state owned by the receiving side. Every successful host-owned mutation consumes exactly one globally increasing revision:

- `received == current + 1`: apply atomically and advance.
- `received <= current`: ignore as duplicate or stale.
- `received > current + 1`: do not apply; enter synchronization and send `sync_request`.

## Connection messages

| Type | Direction | Required payload |
| --- | --- | --- |
| `hello` | either | `device_id`, `session_id`, `runtime_version`, `protocol_versions`, `simulator` |
| `capabilities` | either | `features`, `apps`, `display` |
| `ping` | either | `nonce` |
| `pong` | either | `nonce`; `reply_to` references the ping |
| `sync_request` | watch → host | `last_revision` |
| `sync_response` | host → watch | `revision`, `state` |

## State updates

| Type | Required payload | Meaning |
| --- | --- | --- |
| `state_set` | `path`, `value`, `revision` | Completely replace the value/subtree at `path`; existing children disappear |
| `state_patch` | `path`, `value`, `revision` | Replace exactly one registered leaf value; object merge is forbidden in v1 |
| `list_insert` | `path`, `index`, `value`, `revision` | Insert into a bounded list |
| `list_remove` | `path`, `index`, `revision` | Remove one list item |
| `list_update` | `path`, `index`, `value`, `revision` | Replace one list item |

List indices are zero-based. Invalid paths, indices, or revisions are handled atomically under the sequencing rules above. Domain list objects should carry stable IDs even when index controls display order, so actions reference `notif_9473` rather than a potentially shifted index.

## Host commands

| Type | Required payload | Meaning |
| --- | --- | --- |
| `refresh` | `scope` | Request background refresh |
| `app_open` | `app_id` | Ask the watch to open a registered app |
| `show_toast` | `message`, `duration_ms` | Show bounded transient feedback |
| `show_dialog` | `title`, `message`, `actions` | Show a modal with semantic action IDs |
| `cache_invalidate` | `path` | Mark a cache subtree stale |

## Command responses

`command_result` is the standardized response to commands. Its envelope `reply_to` identifies the command. Payload `status` is `ok`, `rejected`, or `error`; `code` is machine-readable. Initial codes are `unsupported`, `invalid_argument`, `app_not_found`, `busy`, `not_available`, and `permission_denied`. Additive diagnostic fields may be ignored.

## Watch events

| Type | Required payload | Meaning |
| --- | --- | --- |
| `app_opened` | `app_id` | Completed local navigation event |
| `action` | `app_id`, `action_id`, `arguments` | Semantic user action |
| `button` | `button`, `gesture` | Normalized hardware-button event |
| `gesture` | `gesture` | Normalized swipe/tap/long-press event |
| `input` | `input_id`, `value` | Structured input value |
| `refresh_request` | `scope` | Watch requests updated host data |

## Limits

- Maximum encoded packet: 16 KiB.
- Maximum toast text: 160 UTF-8 bytes.
- Maximum dialog actions: 4.
- Maximum state path: 128 ASCII characters.
- Receivers independently bound list, string, and nesting allocations.
- Sensitive data is not cached without an explicit application requirement.

## TCP framing

- Encoding is UTF-8.
- One complete JSON object occupies one frame.
- Frames end with LF (`0x0A`); receivers may accept CRLF.
- The maximum frame is 16,384 bytes excluding the delimiter.
- Empty frames are ignored.
- Invalid UTF-8 is a protocol error.
- Newlines within JSON strings use JSON escaping and never appear literally on the wire.

## Reconnection

1. Exchange `hello` and select the highest mutual semantic version.
2. Exchange `capabilities`.
3. Watch enters `SYNCING` and sends `sync_request` with its last revision.
4. Incremental mutations received while syncing are buffered and not applied.
5. Host returns a consistent `sync_response` snapshot at revision `N`.
6. Watch atomically replaces all host-owned state and sets `current_revision = N`.
7. Watch applies buffered mutations `N+1`, `N+2`, and onward in strict order.
8. When the buffer is contiguous, watch enters `SYNCHRONIZED`; otherwise it requests sync again.

Ordinary watch navigation proceeds throughout synchronization.

## Error policy

Malformed JSON, unknown types, invalid envelopes, impossible indices, and unsupported versions are logged and rejected. They never partially mutate state. Repeated framing errors cause a transport disconnect and clean resynchronization.
