# AGENTS.md — playos-platform-api

Guidance for AI agents and contributors working in this repository.

## What this repository is

The reference implementation of the **PlayOS Platform API**. It implements the
contracts defined in `playos-spec`. It is **not** the source of truth for
platform behavior — the specification is.

## Golden rules

1. **Engine agnostic.** Public headers under `include/playos/` MUST NOT include
   any game-engine header (no Raylib, SDL, Godot). No API may require a
   specific engine.
2. **Spec first.** If a change alters platform behavior, update `playos-spec`
   (book/RFC) first, then implement here.
3. **Capabilities over platform checks.** Do not branch on the operating
   system in public API. Expose optional features through the capability
   model and let backends provide them.
4. **Backends are swappable.** Platform-specific code lives under
   `src/backends/<platform>/` behind the interfaces in
   `src/backends/*.h`. Selecting a backend is a CMake concern.
5. **Stable public surface.** Treat `include/playos/` as a contract. Additive
   changes are preferred; breaking changes need a spec change.

## Layout

| Path | Purpose |
|---|---|
| `include/playos/` | Public Platform API headers |
| `src/*.cpp` | Engine-agnostic implementation |
| `src/backends/` | Platform-selected backends (Windows, null, later Linux) |
| `CMakeLists.txt` | Build definition; selects the backend |

## Adding a backend

1. Create `src/backends/<platform>/<name>_backend.cpp` implementing the
   relevant interface from `src/backends/`.
2. Add a branch in `CMakeLists.txt` that compiles it for that platform.
3. Do not add platform `#ifdef`s to the public headers.

## Build

```sh
cmake -B build
cmake --build build
```
