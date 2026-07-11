# PlayOS Platform API — Roadmap

Feature tracking and development plan for `playos-platform-api`, the
engine-agnostic C++ library that provides all OS services to the shell
and games.

The Platform API is the **only** place where OS-specific code lives.
Backends follow an interface+factory pattern (`IBatteryBackend`,
`CreateBatteryBackend()`) so the shell never contains `#ifdef __linux__`.

---

## ✅ Completed (v0.2)

### Core backends
- [x] Input: Linux evdev (gamepad buttons/axes via `/dev/input/event*`)
- [x] Input: Raylib (keyboard/gamepad for Windows dev)
- [x] Input: Windows XInput (gamepad)
- [x] Display: Linux (DRM modesetting via compositor)
- [x] Display: Raylib (windowed for Windows dev)
- [x] Audio: Raylib (cross-platform, via raylib audio)
- [x] Touch: Raylib (cross-platform)
- [x] Storage: Linux (XDG paths)
- [x] Storage: Windows (known folders)
- [x] Lifecycle: Raylib (init/shutdown)
- [x] Runtime: Linux (fork+exec process launch)
- [x] Capabilities model: `Capability` enum + `HasCapability()`

### Extended backends
- [x] Battery: Linux sysfs (`/sys/class/power_supply/`)
- [x] Battery: Windows (`GetSystemPowerStatus`)
- [x] Battery: Null (fallback)
- [x] Network: Linux (`getifaddrs`, `nmcli` for scan/connect)
- [x] Network: Windows (`GetAdaptersAddresses`)
- [x] Network: Null (fallback)
- [x] Bluetooth: Linux sysfs (`/sys/class/bluetooth/`)
- [x] Bluetooth: Windows (`BluetoothFindFirstRadio`)
- [x] Bluetooth: Null (fallback)

### Input button mapping (v0.2 hardcoded)
- [x] `BTN_MODE` → Home (standard Xbox guide)
- [x] `BTN_TRIGGER_HAPPY1`–`HAPPY4` → Home (vendor shotgun)
- [x] `KEY_PROG1` → Home, `KEY_PROG2` → QuickSettings

---

## 📋 Planned — Phase 2.5 (Device Profiles · RFC-0006)

> **Goal:** Replace hardcoded button mappings with runtime-loaded TOML
> profiles. Unlocks multi-device support without recompilation.
>
> Spec: [`rfcs/0006-device-profile-format.md`](https://github.com/PlayOS-Foundation/playos-spec/blob/main/rfcs/0006-device-profile-format.md)

### TOML integration
- [ ] Integrate `tomlplusplus` (header-only, CMake FetchContent)
- [ ] DeviceProfile C++ class — load, validate, expose typed accessors

### Input mapping
- [ ] `InputMapping` lookup table: symbolic names → Linux evdev key codes
  - Vocabulary: `asus_armoury`, `asus_command_center`, `steamdeck_qam`,
    `xbox_guide`, `legion_quick`, etc.
- [ ] Profile-aware `CreateInputBackend(profilePath)` — uses profile for
      button mapping; falls back to hardcoded defaults if no profile found
- [ ] Profile-aware `CreateBatteryBackend(profilePath)` — skips battery
      polling if profile says `"power.battery" = false`

### Profile loading
- [ ] Auto-detect: check kernel cmdline `playos.profile=`, then DMI
      product name, then fallback to generic
- [ ] Profile path: `/etc/playos/device-profiles/<id>.toml`

---

## 📋 Planned — Phase 3 (New APIs)

- [ ] `Brightness` API — get/set display brightness
  - Linux backend: `/sys/class/backlight/`
  - Windows backend: `SetMonitorBrightness`
- [ ] `Power` API — sleep, shutdown, restart
- [ ] `Fan` API — fan speed / thermal profile (ROG Ally, Steam Deck)
- [ ] `TDP` API — TDP control for handhelds
- [ ] `Update` API — OTA updates (download + apply)
- [ ] `Bluetooth` scan/pair — extend beyond `IsPresent()`

---

## Architecture reference

See `playos-spec` book, chapter
[`08-runtime-architecture/04-linux-reference-runtime.md`](https://github.com/PlayOS-Foundation/playos-spec/blob/main/book/src/08-runtime-architecture/04-linux-reference-runtime.md)
for the runtime architecture.

See `ARCHITECTURE.md` in this repo for backend interface conventions.
