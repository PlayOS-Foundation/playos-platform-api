# playos-platform-api

Reference implementation of the **PlayOS Platform API** — the engine-agnostic
services (input, storage, lifecycle, capabilities, and more) that PlayOS
applications call.

This repository implements contracts defined in
[`playos-spec`](https://github.com/PlayOS-Foundation/playos-spec). Platform
behavior is specified there first; this repository implements it. It must not
depend on any game engine (see the "Engine Agnostic" principle).

## Status

Early scaffold. Currently provides a minimal vertical-slice surface:

- **Capabilities** — `PlayOS::Capabilities::Has/List/Id` (RFC-0003).
- **Input** — logical buttons/axes via `PlayOS::Input` (XInput backend on
  Windows, null backend elsewhere).
- **Lifecycle** — `PlayOS::Lifecycle::Init/Update/Shutdown`.

## Layout

```text
include/playos/      Public headers (the Platform API contract)
  playos.h           Umbrella header + version
  capabilities.h
  input.h
  lifecycle.h
src/                 Implementation
  capabilities.cpp
  input.cpp
  lifecycle.cpp
  backends/          Platform-selected backends
    input_backend.h  Internal backend interface
    windows/         XInput backend (primary SDK target)
    null/            Stub backend for other platforms
```

## Building

Requires CMake >= 3.20 and a C++17 compiler.

```sh
cmake -B build
cmake --build build
```

On Windows this builds with the installed Visual Studio toolchain and links
`Xinput`. On other platforms it compiles with the null backend until a native
backend is added.

## Consuming it

Other PlayOS repositories (for example `playos-shell`) link the
`PlayOS::PlatformAPI` target, typically via `add_subdirectory` of a sibling
checkout or `FetchContent`.

## License

Code in this repository will be released under an OSI-approved license
(MIT/Apache-2.0); the specification in `playos-spec` is CC-BY-4.0.
