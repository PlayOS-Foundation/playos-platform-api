# AGENTS.md — playos-platform-api

This repository contains **libplayos** — the public C ABI that game developers use to interact with the PlayOS runtime. It is the only interface games are allowed to use; they must not call into the compositor, init, or shell directly.

## Specification Reference

Before touching any header or source file, read:
- [`playos-spec/src/platform-api.md`](https://github.com/PlayOS-Foundation/playos-spec/blob/main/src/platform-api.md) — canonical API spec (all signatures, semantics, error codes)
- [`playos-spec/src/architecture.md`](https://github.com/PlayOS-Foundation/playos-spec/blob/main/src/architecture.md) — system context

## Repository Layout

```
include/playos/
├── playos.h            ← Master include (include this, not individual headers)
├── playos_system.h     ← Init / shutdown / version
├── playos_lifecycle.h  ← Lifecycle events (fd-based)
├── playos_input.h      ← Gamepad, button bitmasks, axis arrays
├── playos_display.h    ← Display info queries
├── playos_storage.h    ← Save-game paths, quota
├── playos_audio.h      ← Audio stream open/write/close
├── playos_power.h      ← Battery, TDP, suspend hint
└── playos_logging.h    ← Structured log output

src/
├── playos_system.c     ← Implementation stubs
├── playos_lifecycle.c
├── playos_input.c
├── playos_display.c
├── playos_storage.c
├── playos_audio.c
├── playos_power.c
├── playos_logging.c
└── backend/
    ├── backend_stub.c  ← No-op backend (testing / host dev)
    └── backend_playos.c← Real backend (links to runtime IPC)

tests/
└── CMakeLists.txt

CMakeLists.txt
```

## ABI Rules — Critical

- **SONAME is `libplayos.so.0`**. The minor version may increment; the SONAME must not change until a breaking ABI change is intentional and documented in a new ADR.
- **Never remove or rename a public symbol** from any header in `include/playos/`. Deprecate with `PLAYOS_DEPRECATED` macro instead.
- **Never change a function signature** (parameter types, return type, order) without a new ADR and major SONAME bump.
- **`PLAYOS_API_VERSION`** in `playos.h` must be incremented for any additive change to the public API.
- All public functions must be declared `PLAYOS_API` (the visibility macro defined in `playos.h`).

## Code Conventions

- **Standard**: C99. No C11 atomics or VLAs.
- **Naming**: `playos_` prefix for all public symbols. `playos__` (double underscore) for internal-only symbols.
- **Headers**: `extern "C"` guards in every public header for C++ compatibility.
- **Comments**: Doxygen-style (`/** ... */`) on every public function — `@brief`, `@param`, `@return`, `@note` as needed.
- **Errors**: functions return `playos_result_t`. Never return raw errno. Never abort() or exit().
- **No dynamic allocation in the hot path**: input polling and audio write must be allocation-free.

## Build Commands

```sh
cmake -B build -DPLAYOS_BACKEND=stub   # host dev / testing
cmake --build build
ctest --test-dir build
```

## What NOT to Do

- Do not add game logic or UI code here — this is a pure API boundary library.
- Do not link against wlroots, libdrm, or any compositor-side library from this repo.
- Do not read from `/run/playos/` directly in the stub backend — use the IPC helpers from `playos-runtime` when wiring the real backend.
- Do not introduce threading inside libplayos — the API is single-threaded from the game's perspective.
