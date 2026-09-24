# Protocol Assets

- `schema/message.schema.json` defines the strict v1 envelope and registered types.
- `examples/` contains representative valid packets.
- `fixtures/` defines codec-neutral input and expected semantic outcomes shared by C and Rust.
- `tests/test_protocol.py` validates examples and negative compatibility cases.

Run `./scripts/test-protocol.sh` from the repository root.
