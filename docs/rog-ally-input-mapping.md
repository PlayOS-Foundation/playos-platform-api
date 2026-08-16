# ROG Ally Controller Input Mapping

ASUS ROG Ally (2023, Ryzen Z1 Extreme) built-in controller input mapping for PlayOS.

## Device Identification

The Ally exposes its controls across up to **four** evdev nodes, driven by a mix of kernel drivers:

| Node | Driver(s) | Capability signature | Purpose |
|---|---|---|---|
| Gamepad | `xpad` / `hid-asus` | four stick axes (`ABS_X`, `ABS_Y`, `ABS_RX`, `ABS_RY`) + `BTN_SOUTH` | Face buttons, sticks, triggers, d-pad |
| Home | `hid-asus` | `BTN_MODE` without `BTN_SOUTH` | Xbox/Guide button |
| Vendor | `hid-asus-ally` | `KEY_PROG1`/`KEY_PROG2` and/or `BTN_TRIGGER_HAPPY1`/`BTN_TRIGGER_HAPPY2` (also `KEY_VOLUMEUP`/`KEY_VOLUMEDOWN`), without `BTN_SOUTH`/`BTN_MODE` | Armoury Crate, Command Center, hardware volume keys |
| Power | ACPI `PNP0C0C` | `KEY_POWER`/`KEY_SLEEP` without `BTN_SOUTH`/`BTN_MODE` | Hardware power/sleep button (shell-owned only) |

The platform-api backend and the trusted shell both scan `/dev/input/event*`. The backend opens the gamepad node as the primary device and best-effort opens the home and vendor nodes for reserved buttons. The shell additionally owns the hardware volume keys on the vendor node and the ACPI power/sleep node; the backend intentionally ignores them.

---

## Button Mapping (EV_KEY)

| Physical control | evdev code | Value | PlayOS button | Public? |
|---|---|---|---|---|
| A (south) | `BTN_SOUTH` | 0x130 | `PLAYOS_BUTTON_SOUTH` (bit 0) | ✅ |
| B (east) | `BTN_EAST` | 0x131 | `PLAYOS_BUTTON_EAST` (bit 1) | ✅ |
| X (west) | `BTN_WEST` | 0x133 | `PLAYOS_BUTTON_WEST` (bit 2) | ✅ |
| Y (north) | `BTN_NORTH` | 0x132 | `PLAYOS_BUTTON_NORTH` (bit 3) | ✅ |
| Menu (Start) | `BTN_START` | 0x13b | `PLAYOS_BUTTON_START` (bit 4) | ✅ |
| View (Select) | `BTN_SELECT` | 0x13a | `PLAYOS_BUTTON_SELECT` (bit 5) | ✅ |
| Xbox/Guide | `BTN_MODE` | 0x13c | `PLAYOS_BUTTON_SYSTEM` (bit 6) | ❌ Reserved |
| Armoury Crate (alt) | `KEY_PROG1` | 0x94 | `PLAYOS_BUTTON_SYSTEM` (bit 6) | ❌ Reserved |
| Armoury Crate (alt) | `BTN_TRIGGER_HAPPY1` | 0x2c0 | `PLAYOS_BUTTON_SYSTEM` (bit 6) | ❌ Reserved |
| Command Center (alt) | `KEY_PROG2` | 0x95 | `PLAYOS_BUTTON_QUICK_MENU` (bit 7) | ❌ Reserved |
| Command Center (alt) | `BTN_TRIGGER_HAPPY2` | 0x2c1 | `PLAYOS_BUTTON_QUICK_MENU` (bit 7) | ❌ Reserved |
| D-pad Up | `BTN_DPAD_UP` / `ABS_HAT0Y` | 0x220 / ABS 17 | `PLAYOS_BUTTON_DPAD_UP` (bit 8) | ✅ |
| D-pad Down | `BTN_DPAD_DOWN` / `ABS_HAT0Y` | 0x221 / ABS 17 | `PLAYOS_BUTTON_DPAD_DOWN` (bit 9) | ✅ |
| D-pad Left | `BTN_DPAD_LEFT` / `ABS_HAT0X` | 0x222 / ABS 16 | `PLAYOS_BUTTON_DPAD_LEFT` (bit 10) | ✅ |
| D-pad Right | `BTN_DPAD_RIGHT` / `ABS_HAT0X` | 0x223 / ABS 16 | `PLAYOS_BUTTON_DPAD_RIGHT` (bit 11) | ✅ |
| LB (L1) | `BTN_TL` | 0x136 | `PLAYOS_BUTTON_L1` (bit 12) | ✅ |
| RB (R1) | `BTN_TR` | 0x137 | `PLAYOS_BUTTON_R1` (bit 13) | ✅ |
| Left Stick Click (L3) | `BTN_THUMBL` | 0x13d | `PLAYOS_BUTTON_L3` (bit 14) | ✅ |
| Right Stick Click (R3) | `BTN_THUMBR` | 0x13e | `PLAYOS_BUTTON_R3` (bit 15) | ✅ |
| Power button | `KEY_POWER` / `KEY_SLEEP` | 0x74 / 0x8e (142) | `PLAYOS_BUTTON_POWER` (bit 16) | ❌ Reserved |

### Reserved Buttons

- `PLAYOS_BUTTON_SYSTEM` is produced by `BTN_MODE`, `KEY_PROG1`, and `BTN_TRIGGER_HAPPY1`.
- `PLAYOS_BUTTON_QUICK_MENU` is produced by `KEY_PROG2`, `BTN_TRIGGER_HAPPY2`, and (in the shell) `KEY_LEFTMETA`/`KEY_RIGHTMETA`.
- `PLAYOS_BUTTON_POWER` is produced by `KEY_POWER`/`KEY_SLEEP` on the ACPI Power/Sleep nodes. It is shell-owned: the backend never opens those nodes, and the public API additionally strips the bit as a defensive guarantee.

All three are **stripped** by the public API layer before game-facing snapshots. The compositor and shell may observe them for system-level actions (home screen, overlay, power), but games must never see them.

The hardware volume keys (`KEY_VOLUMEUP` / `KEY_VOLUMEDOWN`) are **not** PlayOS buttons and are handled directly by the trusted shell, which adjusts the system master volume through `playos_audio_*`. The platform-api backend intentionally leaves them unmapped.

---

## Axis Mapping (EV_ABS)

| Physical axis | evdev code | Raw range | Normalized | PlayOS axis |
|---|---|---|---|---|
| Left Stick X | `ABS_X` (0) | [-32768, 32767] | [-1.0, 1.0] | `PLAYOS_AXIS_LEFT_X` |
| Left Stick Y | `ABS_Y` (1) | [-32768, 32767] | [-1.0, 1.0] | `PLAYOS_AXIS_LEFT_Y` |
| Right Stick X | `ABS_RX` (3) | [-32768, 32767] | [-1.0, 1.0] | `PLAYOS_AXIS_RIGHT_X` |
| Right Stick Y | `ABS_RY` (4) | [-32768, 32767] | [-1.0, 1.0] | `PLAYOS_AXIS_RIGHT_Y` |
| Left Trigger | `ABS_Z` (2) | [0, 255] or [0, 1023] | [0.0, 1.0] | `PLAYOS_AXIS_LEFT_TRIGGER` |
| Right Trigger | `ABS_RZ` (5) | [0, 255] or [0, 1023] | [0.0, 1.0] | `PLAYOS_AXIS_RIGHT_TRIGGER` |

### Axis Conventions

- **Sticks:** Range [-1.0, 1.0]. Y axes: negative = up (matches evdev convention).
- **Triggers:** Range [0.0, 1.0]. 0.0 = released, 1.0 = fully pressed.
- **Dead zone:** ±5% on sticks (configurable via `STICK_DEADZONE` in backend).
- **Trigger max:** Auto-detected from `EVIOCGABS(ABS_Z)` on device open. Handles both 8-bit (255) and 10-bit (1023) trigger ranges.

---

## Ally-Specific Quirks

### D-Pad Dual Representation

Some kernel drivers (xpad) report D-pad as both `EV_KEY` events (`BTN_DPAD_*`) **and** `EV_ABS` hat events (`ABS_HAT0X`/`ABS_HAT0Y`). The two layers handle these differently:

- **Platform API backend** (`backend_evdev.c`) maps **only** the `ABS_HAT0X`/`ABS_HAT0Y` hat path. `BTN_DPAD_*` events are intentionally ignored there because xpad/hid-asus report both, and the hat path enforces mutually exclusive directions (LEFT clears RIGHT, UP clears DOWN).
- **Trusted shell** (`src/input.c`) handles **both** forms (`BTN_DPAD_*` and `ABS_HAT0*`), writing into the same button bitmap.

Both representations ultimately drive the same logical D-pad buttons, so games receive consistent state regardless of which event path the driver emits.

### Armoury Crate / Command Center

The two small buttons below the screen live on the vendor node (`hid-asus-ally`), not the gamepad node:

- Armoury Crate (`KEY_PROG1`, with `BTN_TRIGGER_HAPPY1` as an observed alternate) maps to `PLAYOS_BUTTON_SYSTEM`.
- Command Center (`KEY_PROG2`, with `BTN_TRIGGER_HAPPY2` as an observed alternate) maps to `PLAYOS_BUTTON_QUICK_MENU`.

On `xpad` these may not be exposed, or may appear under a different code. Both are reserved and stripped from game snapshots. The exact `KEY_PROG*` vs `BTN_TRIGGER_HAPPY*` pairing is per-firmware and may vary, so the backend accepts either code for the same logical button.

### Power / Sleep Button

The hardware power button arrives on a dedicated ACPI `PNP0C0C` node and reports `KEY_POWER` (`0x74`) or `KEY_SLEEP` (`0x8e` = 142). Like HOME/COMMAND, it is a momentary pulse: press and release arrive within a few milliseconds even when the button is physically held, with no autorepeat. The shell therefore latches the visual state for ~0.6s in the Live Input Test so a press is clearly visible, while the actual input is treated as a level bit set for the one frame the pulse is observed.

### Gyro / IMU

The Ally has an integrated IMU (accelerometer + gyroscope). This is **not** exposed through the standard gamepad evdev interface and is out of scope for Sprint 3. Future sprints may add gyro-as-aux-input support via IIO or a dedicated driver.

### Back Paddles (M1/M2)

The ROG Ally has two rear macro buttons (M1/M2). They *may* appear as `BTN_TRIGGER_HAPPY3`/`BTN_TRIGGER_HAPPY4` depending on the driver — this is **unverified** on the current hid-asus firmware. They are **not mapped** in the current public contract but are noted here for future inclusion.

---

## Verification

```sh
# List input devices
cat /proc/bus/input/devices

# Monitor raw controller events
evtest /dev/input/eventX

# Check device capabilities
evtest --info /dev/input/eventX

# Run PlayOS hardware verification
sh tools/hw-check/check-input.sh
```

---

## Deferred to Future Sprints

| Feature | Reason |
|---|---|
| Gyro/IMU input | Requires IIO or custom driver |
| Back paddles (M1/M2) | Not yet in public contract; needs mapping standard |
| Touch screen | Separate evdev node; not a game controller |
| RGB lighting control | Out of scope for input API; belongs in platform config |
| Rumble / haptics | Needs `FF` (force feedback) evdev API; deferred |
