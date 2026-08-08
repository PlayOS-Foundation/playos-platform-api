# ROG Ally Controller Input Mapping

ASUS ROG Ally (2023, Ryzen Z1 Extreme) built-in controller input mapping for PlayOS.

## Device Identification

The Ally controller appears as an Xbox-compatible HID device through one of these kernel drivers:

| Driver | Kernel config | Typical evdev node |
|---|---|---|
| `xpad` | `CONFIG_JOYSTICK_XPAD` | `/dev/input/event*` (joystick) |
| `hid-asus` | `CONFIG_HID_ASUS` | `/dev/input/event*` |

The evdev backend auto-discovers the controller by scanning `/dev/input/event*` for a device with both `EV_KEY` and `EV_ABS` capabilities plus the four stick axes (`ABS_X`, `ABS_Y`, `ABS_RX`, `ABS_RY`).

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
| Armoury Crate | `BTN_TRIGGER_HAPPY1` | 0x2c0 | `PLAYOS_BUTTON_QUICK_MENU` (bit 7) | ❌ Reserved |
| D-pad Up | `BTN_DPAD_UP` / `ABS_HAT0Y` | 0x220 / ABS 17 | `PLAYOS_BUTTON_DPAD_UP` (bit 8) | ✅ |
| D-pad Down | `BTN_DPAD_DOWN` / `ABS_HAT0Y` | 0x221 / ABS 17 | `PLAYOS_BUTTON_DPAD_DOWN` (bit 9) | ✅ |
| D-pad Left | `BTN_DPAD_LEFT` / `ABS_HAT0X` | 0x222 / ABS 16 | `PLAYOS_BUTTON_DPAD_LEFT` (bit 10) | ✅ |
| D-pad Right | `BTN_DPAD_RIGHT` / `ABS_HAT0X` | 0x223 / ABS 16 | `PLAYOS_BUTTON_DPAD_RIGHT` (bit 11) | ✅ |
| LB (L1) | `BTN_TL` | 0x136 | `PLAYOS_BUTTON_L1` (bit 12) | ✅ |
| RB (R1) | `BTN_TR` | 0x137 | `PLAYOS_BUTTON_R1` (bit 13) | ✅ |
| Left Stick Click (L3) | `BTN_THUMBL` | 0x13d | `PLAYOS_BUTTON_L3` (bit 14) | ✅ |
| Right Stick Click (R3) | `BTN_THUMBR` | 0x13e | `PLAYOS_BUTTON_R3` (bit 15) | ✅ |

### Reserved Buttons

`PLAYOS_BUTTON_SYSTEM` (Xbox/Guide) and `PLAYOS_BUTTON_QUICK_MENU` (Armoury Crate) are **stripped** by the public API layer before game-facing snapshots. The compositor and shell may observe them for system-level actions (home screen, overlay), but games must never see them.

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

Some kernel drivers (xpad) report D-pad as both `EV_KEY` events (`BTN_DPAD_*`) **and** `EV_ABS` hat events (`ABS_HAT0X`/`ABS_HAT0Y`). The backend handles both paths — if `BTN_DPAD_*` events arrive, they set the bitmask directly. If `ABS_HAT0*` events arrive, the backend translates hat values to button bits. This is safe because both representations update the same button bitmap.

### Armoury Crate Button

The small button below the screen (Armoury Crate / Command Center) maps to `BTN_TRIGGER_HAPPY1` on hid-asus. On xpad this button may not be exposed or may appear under a different code. This button is reserved as `PLAYOS_BUTTON_QUICK_MENU` and stripped from game snapshots.

### Gyro / IMU

The Ally has an integrated IMU (accelerometer + gyroscope). This is **not** exposed through the standard gamepad evdev interface and is out of scope for Sprint 3. Future sprints may add gyro-as-aux-input support via IIO or a dedicated driver.

### Back Paddles (M1/M2)

The ROG Ally has two rear macro buttons (M1/M2). These may appear as `BTN_TRIGGER_HAPPY3`/`BTN_TRIGGER_HAPPY4` depending on the driver. They are **not mapped** in the current public contract but are noted here for future inclusion.

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
