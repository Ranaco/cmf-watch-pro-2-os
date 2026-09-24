#!/usr/bin/env python3
import json
from pathlib import Path
from jsonschema import Draft202012Validator

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = json.loads((ROOT / "schema/message.schema.json").read_text())
VALIDATOR = Draft202012Validator(SCHEMA)

REQUIRED_PAYLOAD_FIELDS = {
    "hello": {"device_id", "runtime_version", "protocol_versions", "simulator"},
    "capabilities": {"features", "apps", "display"},
    "ping": {"nonce"}, "pong": {"nonce"},
    "sync_request": {"last_revision"}, "sync_response": {"revision", "state"},
    "state_set": {"path", "value", "revision"},
    "state_patch": {"path", "value", "revision"},
    "list_insert": {"path", "index", "value", "revision"},
    "list_remove": {"path", "index", "revision"},
    "list_update": {"path", "index", "value", "revision"},
    "refresh": {"scope"}, "app_open": {"app_id"},
    "show_toast": {"message", "duration_ms"},
    "show_dialog": {"title", "message", "actions"},
    "cache_invalidate": {"path"}, "app_opened": {"app_id"},
    "action": {"app_id", "action_id", "arguments"},
    "button": {"button", "gesture"}, "gesture": {"gesture"},
    "input": {"input_id", "value"}, "refresh_request": {"scope"},
}

def validate_message(message: dict) -> None:
    VALIDATOR.validate(message)
    missing = REQUIRED_PAYLOAD_FIELDS[message["type"]] - message["payload"].keys()
    assert not missing, f"{message['type']} missing fields: {sorted(missing)}"

def main() -> None:
    registered_types = set(SCHEMA["properties"]["type"]["enum"])
    assert registered_types == REQUIRED_PAYLOAD_FIELDS.keys(), \
        "schema types and semantic payload contracts diverged"

    examples = []
    for path in sorted((ROOT / "examples").glob("*.json")):
        message = json.loads(path.read_text())
        validate_message(message)
        examples.append(message)
    ids = [message["id"] for message in examples]
    assert len(ids) == len(set(ids)), "example message IDs must be unique"

    invalid = [
        {"version": 2, "type": "ping", "id": 1, "payload": {"nonce": 1}},
        {"version": 1, "type": "draw_pixels", "id": 1, "payload": {}},
        {"version": 1, "type": "ping", "id": -1, "payload": {"nonce": 1}},
        {"version": 1, "type": "ping", "id": 1, "payload": {}, "extra": True},
    ]
    for message in invalid:
        assert list(VALIDATOR.iter_errors(message)), f"invalid envelope accepted: {message}"

    incomplete = {"version": 1, "type": "state_patch", "id": 9,
                  "payload": {"path": "weather.temperature", "value": 28}}
    try:
        validate_message(incomplete)
    except AssertionError:
        pass
    else:
        raise AssertionError("state_patch without revision was accepted")
    print(f"Protocol v1 tests passed: {len(examples)} examples and negative cases.")

if __name__ == "__main__":
    main()
