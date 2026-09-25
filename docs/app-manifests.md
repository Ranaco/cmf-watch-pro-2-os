# Compiled Application Manifests

Each application has one JSON file under `apps/manifests` with these required fields:

- `id`: stable protocol/application identity
- `name`: local display name
- `version`: positive manifest version
- `entry`: native entry-view identity
- `screen`: one unique compiled `WatchScreen`
- `headline` and `offline_message`: bounded local copy
- `offline_available`: local availability contract

Run `scripts/generate-app-registry.py` to generate `watch/src/apps/app_registry.generated.inc`. The generated file is compiled into the native binary and checked for drift in tests. Manifests cannot execute code, allocate widgets, access hardware, or inject host-rendered pixels.
