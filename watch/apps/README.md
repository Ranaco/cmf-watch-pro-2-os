# Watch Applications

The initial app system is a native compiled registry in `src/apps/app_registry.c`.

Each descriptor provides:

- stable app ID and display name
- local screen identity
- offline headline and message
- offline-availability capability

The current registry contains Home, Notifications, Music, Assistant, and Settings. Rendering and navigation remain shared runtime services; apps do not perform host computation or access hardware directly.

Declarative bundles and manifests are intentionally deferred until the runtime and protocol are stable.
