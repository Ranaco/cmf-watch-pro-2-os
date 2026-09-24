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
| `version` | unsigned integer | Exactly `1` |
| `type` | string | Registered snake-case message type |
| `id` | unsigned integer | Sender-local monotonically increasing identifier |
| `payload` | object | Message-specific data |
| `reply_to` | unsigned integer | Optional request ID being answered |
| `timestamp_ms` | unsigned integer | Optional informational sender clock |

Unknown versions, types, fields, and malformed packets are rejected without state mutation.

## State paths and revisions

State paths use dot notation, such as `weather.temperature`. Each host-owned mutation carries a monotonically increasing `revision`. The watch applies only newer revisions; a gap triggers `sync_request`.

## Connection messages

| Type | Direction | Required payload |
| --- | --- | --- |
| `hello` | either | `device_id`, `runtime_version`, `protocol_versions`, `simulator` |
| `capabilities` | either | `features`, `apps`, `display` |
| `ping` | either | `nonce` |
| `pong` | either | `nonce`; `reply_to` references the ping |
| `sync_request` | watch → host | `last_revision` |
| `sync_response` | host → watch | `revision`, `state` |

## State updates

| Type | Required payload | Meaning |
| --- | --- | --- |
| `state_set` | `path`, `value`, `revision` | Replace one state subtree |
| `state_patch` | `path`, `value`, `revision` | Update one state value/subtree |
| `list_insert` | `path`, `index`, `value`, `revision` | Insert into a bounded list |
| `list_remove` | `path`, `index`, `revision` | Remove one list item |
| `list_update` | `path`, `index`, `value`, `revision` | Replace one list item |

List indices are zero-based. Invalid paths, indices, or old revisions are rejected atomically.

## Host commands

| Type | Required payload | Meaning |
| --- | --- | --- |
| `refresh` | `scope` | Request background refresh |
| `app_open` | `app_id` | Ask the watch to open a registered app |
| `show_toast` | `message`, `duration_ms` | Show bounded transient feedback |
| `show_dialog` | `title`, `message`, `actions` | Show a modal with semantic action IDs |
| `cache_invalidate` | `path` | Mark a cache subtree stale |

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

## Reconnection

1. Exchange `hello` and select the highest mutual version.
2. Exchange `capabilities`.
3. Watch sends `sync_request` with its last revision.
4. Host returns a consistent `sync_response` snapshot.
5. Incremental state messages resume after that revision.

Ordinary watch navigation proceeds throughout synchronization.

## Error policy

Malformed JSON, unknown types, invalid envelopes, impossible indices, and unsupported versions are logged and rejected. They never partially mutate state. Repeated framing errors cause a transport disconnect and clean resynchronization.

