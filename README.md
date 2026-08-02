# PlayOS Platform API

> Public `libplayos` C ABI, portable implementations, C++ wrappers, and engine adapters.

**Dependency position:** `playos-platform-api` is a **dependency** of games, `playos-shell`, and `playos-overlay`. It depends on `playos-runtime` (for the internal lifecycle transport backend) and `playos-spec` (for API contracts).

## What This Repository Owns

- All public `include/playos/` C headers
- `libplayos.so` — the authoritative shared library for PlayOS runtime devices
- The Raylib PlayOS backend (`src/backends/rcore_playos.c`)
- Optional C++ wrappers
- Engine adapter documentation and examples

## What It Does NOT Own

- Internal IPC definitions → `playos-runtime`
- Compositor code → `playos-compositor`
- Build system / image assembly → `playos-refdistro`

## Public API Groups

| Header | Covers |
|---|---|
| `playos_system.h` | Device and OS information |
| `playos_lifecycle.h` | Lifecycle events (foreground, background, terminate) |
| `playos_input.h` | Logical controller input |
| `playos_display.h` | Display resolution and refresh info |
| `playos_storage.h` | Per-game save, cache, and install paths |
| `playos_audio.h` | Audio state and volume control |
| `playos_power.h` | Battery, thermal, performance profiles |
| `playos_logging.h` | Structured logging |

## API Stability

The C ABI is the authoritative compatibility boundary. See [`platform-api.md`](https://github.com/your-org/playos-spec/blob/main/platform-api.md) for the full stability policy.

Current ABI version: **1** (`PLAYOS_API_VERSION = 1`)  
SONAME: `libplayos.so.0`

## Building

```bash
# Host native build (for unit testing without Buildroot)
cmake -S . -B build -DPLAYOS_BACKEND=stub
cmake --build build
ctest --test-dir build

# Cross-compile via Buildroot (normal path)
# See playos-refdistro build-guide.md
```

## Documentation

Full API reference: [`playos-spec/platform-api.md`](https://github.com/your-org/playos-spec/blob/main/platform-api.md)
