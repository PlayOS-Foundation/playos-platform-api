# Copilot instructions — playos-platform-api

This repository is the reference implementation of the **PlayOS Platform API**.
The source of truth for platform behavior is
[`playos-spec`](https://github.com/PlayOS-Foundation/playos-spec); implement,
don't redefine.

Also read `AGENTS.md` in the repo root.

## Rules for changes here

1. **Engine agnostic.** Public headers in `include/playos/` MUST NOT include a
   game-engine header (no Raylib/SDL/Godot). No API may require a specific
   engine.
2. **Capabilities over platform checks.** Do not branch on the OS in public
   API. Expose optional features via the capability model; backends provide
   them.
3. **Backends are swappable.** Platform code lives under
   `src/backends/<platform>/` behind interfaces in `src/backends/`. Backend
   selection is a CMake concern; no OS `#ifdef` in public headers.
4. **Keep it building.** Validate with `cmake -B build && cmake --build build`
   (MSVC on Windows / GCC/Clang on Linux).
5. **Spec first.** If a change alters the API contract, update `playos-spec`
   (book/RFC) before or alongside the code.

## Where things go

- Public API: `include/playos/`
- Implementation: `src/*.cpp`
- Backends: `src/backends/{windows,linux,null}/`
