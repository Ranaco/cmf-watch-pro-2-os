cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cmf_watch_simulator)

target_include_directories(app PRIVATE src)

target_sources(app PRIVATE
  src/main.c
  src/runtime/watch_runtime.c
  src/apps/app_registry.c
  src/navigation/navigation.c
  src/renderer/renderer.c
  src/input/watch_input.c
  src/input/watch_gesture.c
  src/state/watch_state.c
  src/state/watch_sync.c
  src/animation/watch_animation.c
  src/cache/watch_cache.c
  src/cache/watch_cache_storage.c
  src/protocol/watch_protocol.c
  src/transport/transport.c
)
