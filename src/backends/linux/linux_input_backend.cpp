// Linux input backend using evdev directly (no external dependencies).
//
// Reads a gamepad from /dev/input/event* and maps evdev codes to logical
// PlayOS buttons/axes. Reading evdev directly (rather than via the Wayland
// seat) means the shell receives controller input even while running as a
// Wayland client of the PlayOS compositor.
//
// This is a Stage-1 backend: it binds the first gamepad it finds. Multi-device
// handling, hotplug, and libinput integration are future work.
#include "backends/input_backend.h"
#include "backends/linux/input_mapping.h"
#include "playos/device_profile.h"

#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace PlayOS {
namespace Detail {
namespace {

bool testBit(const unsigned long* bits, int bit) {
    return (bits[bit / (8 * sizeof(unsigned long))] >>
            (bit % (8 * sizeof(unsigned long)))) &
           1UL;
}

class LinuxInputBackend : public IInputBackend {
public:
    LinuxInputBackend() { openDevices(); LoadProfile(); }
    ~LinuxInputBackend() override {
        if (fd_ >= 0) close(fd_);
        if (homeFd_ >= 0) close(homeFd_);
        if (vendorFd_ >= 0) close(vendorFd_);
        if (traceFile_) fclose(traceFile_);
    }

    void Update() override {
        input_event ev;
        ssize_t n;

        // Read main gamepad device (axes + buttons)
        if (fd_ >= 0) {
            while ((n = read(fd_, &ev, sizeof(ev))) == (ssize_t)sizeof(ev)) {
                if (ev.type == EV_KEY) {
                    onKey(ev.code, ev.value != 0);
                } else if (ev.type == EV_ABS) {
                    onAbs(ev.code, ev.value);
                }
            }
        }

        // Read separate home-button device (only EV_KEY, no axes)
        if (homeFd_ >= 0) {
            while ((n = read(homeFd_, &ev, sizeof(ev))) == (ssize_t)sizeof(ev)) {
                if (ev.type == EV_KEY) {
                    onKey(ev.code, ev.value != 0);
                }
            }
        }

        // Read vendor-specific button device (hid-asus-ally, etc.).
        // These buttons (Armoury Crate, Command Center, back paddles)
        // appear on a separate HID interface from the main Xbox gamepad.
        if (vendorFd_ >= 0) {
            while ((n = read(vendorFd_, &ev, sizeof(ev))) == (ssize_t)sizeof(ev)) {
                if (ev.type == EV_KEY) {
                    onKey(ev.code, ev.value != 0);
                }
            }
        }
    }

    bool Down(Button button) override {
        return buttons_[static_cast<int>(button)];
    }

    float GetAxis(Axis axis) override {
        return axes_[static_cast<int>(axis)];
    }

private:
    void set(Button b, bool v) { buttons_[static_cast<int>(b)] = v; }

    void onKey(int code, bool pressed) {
        // Trace ALL key events for debugging — write to /tmp/playos-input.log
        traceKey(code, pressed);

        // Standard gamepad buttons (same on every device)
        switch (code) {
            case BTN_SOUTH:  set(Button::A, pressed); break;
            case BTN_EAST:   set(Button::B, pressed); break;
            case BTN_WEST:   set(Button::X, pressed); break;
            case BTN_NORTH:  set(Button::Y, pressed); break;
            case BTN_TL:     set(Button::L1, pressed); break;
            case BTN_TR:     set(Button::R1, pressed); break;
            case BTN_TL2:    set(Button::L2, pressed); break;
            case BTN_TR2:    set(Button::R2, pressed); break;
            case BTN_SELECT: set(Button::Select, pressed); break;
            case BTN_START:  set(Button::Start, pressed); break;
            // Some drivers (xpad on certain kernels) report D-Pad as buttons
            case BTN_DPAD_UP:    set(Button::DPadUp, pressed); break;
            case BTN_DPAD_DOWN:  set(Button::DPadDown, pressed); break;
            case BTN_DPAD_LEFT:  set(Button::DPadLeft, pressed); break;
            case BTN_DPAD_RIGHT: set(Button::DPadRight, pressed); break;
            default: break;
        }

        // Profile-resolved vendor buttons (Home, QuickSettings).
        // Resolved from symbolic names in the device profile via InputMapping.
        if (std::find(m_homeCodes.begin(), m_homeCodes.end(), code) != m_homeCodes.end())
            set(Button::Home, pressed);
        if (std::find(m_qsCodes.begin(), m_qsCodes.end(), code) != m_qsCodes.end())
            set(Button::QuickSettings, pressed);
    }

    void onAbs(int code, int value) {
        switch (code) {
            case ABS_HAT0X:
                set(Button::DPadLeft, value < 0);
                set(Button::DPadRight, value > 0);
                break;
            case ABS_HAT0Y:
                set(Button::DPadUp, value < 0);
                set(Button::DPadDown, value > 0);
                break;
            case ABS_X:  axes_[(int)Axis::LeftX] = normStick(ABS_X, value); break;
            case ABS_Y:  axes_[(int)Axis::LeftY] = -normStick(ABS_Y, value); break;
            case ABS_RX: axes_[(int)Axis::RightX] = normStick(ABS_RX, value); break;
            case ABS_RY: axes_[(int)Axis::RightY] = -normStick(ABS_RY, value); break;
            case ABS_Z:  axes_[(int)Axis::LeftTrigger] = normTrigger(ABS_Z, value); break;
            case ABS_RZ: axes_[(int)Axis::RightTrigger] = normTrigger(ABS_RZ, value); break;
            default: break;
        }
    }

    // Normalize a stick axis to [-1, 1] using the device's reported range.
    // Applies a 15% deadzone to filter out resting noise.
    float normStick(int code, int value) const {
        const auto& a = abs_[code];
        if (a.maximum == a.minimum) return 0.0f;
        const float t = (float)(value - a.minimum) / (float)(a.maximum - a.minimum);
        float v = t * 2.0f - 1.0f;
        // Deadzone: squash values within ±deadzone to zero, rescale outer range
        constexpr float deadzone = 0.15f;
        if (v > -deadzone && v < deadzone) return 0.0f;
        if (v > 0.0f) return (v - deadzone) / (1.0f - deadzone);
        return (v + deadzone) / (1.0f - deadzone);
    }

    // Normalize a trigger axis to [0, 1].
    float normTrigger(int code, int value) const {
        const auto& a = abs_[code];
        if (a.maximum == a.minimum) return 0.0f;
        return (float)(value - a.minimum) / (float)(a.maximum - a.minimum);
    }

    // Open main gamepad device AND the separate home-button device.
    // On devices like ROG Ally, the Xbox Guide button is on a different
    // /dev/input/event* node than the gamepad axes/buttons.
    void openDevices() {
        DIR* dir = opendir("/dev/input");
        if (!dir) return;
        dirent* entry;

        // Pass 1: find main gamepad (must have BTN_SOUTH).
        while ((entry = readdir(dir)) != nullptr) {
            if (std::strncmp(entry->d_name, "event", 5) != 0) continue;
            const std::string path = std::string("/dev/input/") + entry->d_name;
            const int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            unsigned long keybits[(KEY_MAX / (8 * sizeof(unsigned long))) + 1] = {0};
            if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) >= 0 &&
                testBit(keybits, BTN_SOUTH)) {
                fd_ = fd;
                cacheAbsInfo();
                break;
            }
            close(fd);
        }

        // Pass 2: find home-button device (has BTN_MODE but NOT BTN_SOUTH —
        // the latter avoids re-opening the main gamepad).
        rewinddir(dir);
        while ((entry = readdir(dir)) != nullptr) {
            if (std::strncmp(entry->d_name, "event", 5) != 0) continue;
            const std::string path = std::string("/dev/input/") + entry->d_name;
            const int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            unsigned long keybits[(KEY_MAX / (8 * sizeof(unsigned long))) + 1] = {0};
            if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) >= 0 &&
                testBit(keybits, BTN_MODE) && !testBit(keybits, BTN_SOUTH)) {
                homeFd_ = fd;
                break;
            }
            close(fd);
        }

        // Pass 3: find vendor-specific button device (hid-asus-ally etc.).
        // On ROG Ally, the Armoury Crate (KEY_PROG1), Command Center
        // (KEY_PROG2), and back paddles (BTN_TRIGGER_HAPPY1/2) appear on a
        // separate HID interface that is neither the Xbox gamepad (no
        // BTN_SOUTH) nor the Xbox guide button (no BTN_MODE).
        rewinddir(dir);
        while ((entry = readdir(dir)) != nullptr) {
            if (std::strncmp(entry->d_name, "event", 5) != 0) continue;
            const std::string path = std::string("/dev/input/") + entry->d_name;
            const int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            unsigned long keybits[(KEY_MAX / (8 * sizeof(unsigned long))) + 1] = {0};
            if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) >= 0) {
                bool hasVendorKeys = testBit(keybits, KEY_PROG1) ||
                                     testBit(keybits, BTN_TRIGGER_HAPPY1);
                bool isMainGamepad = testBit(keybits, BTN_SOUTH);
                bool isHomeDevice  = testBit(keybits, BTN_MODE);
                if (hasVendorKeys && !isMainGamepad && !isHomeDevice) {
                    vendorFd_ = fd;
                    break;
                }
            }
            close(fd);
        }
        closedir(dir);
    }

    void cacheAbsInfo() {
        const int codes[] = {ABS_X, ABS_Y, ABS_RX, ABS_RY, ABS_Z, ABS_RZ};
        for (int code : codes) {
            input_absinfo info{};
            if (ioctl(fd_, EVIOCGABS(code), &info) >= 0) {
                abs_[code] = info;
            }
        }
    }

    bool Connected() const override { return fd_ >= 0; }

    // ── Profile-aware button mapping ────────────────────────────────────

    void traceKey(int code, bool pressed) {
        if (!pressed) return; // only log press-down, not release
        if (!traceFile_) {
            traceFile_ = fopen("/tmp/playos-input.log", "a");
            if (!traceFile_) return;
        }
        const char* name = "?";
        // Map common evdev codes to human-readable names
        switch (code) {
            case BTN_SOUTH:  name = "BTN_SOUTH(A)"; break;
            case BTN_EAST:   name = "BTN_EAST(B)"; break;
            case BTN_WEST:   name = "BTN_WEST(X)"; break;
            case BTN_NORTH:  name = "BTN_NORTH(Y)"; break;
            case BTN_TL:     name = "BTN_TL(L1)"; break;
            case BTN_TR:     name = "BTN_TR(R1)"; break;
            case BTN_TL2:    name = "BTN_TL2(L2)"; break;
            case BTN_TR2:    name = "BTN_TR2(R2)"; break;
            case BTN_SELECT: name = "BTN_SELECT"; break;
            case BTN_START:  name = "BTN_START"; break;
            case BTN_MODE:   name = "BTN_MODE"; break;
            case BTN_DPAD_UP:    name = "BTN_DPAD_UP"; break;
            case BTN_DPAD_DOWN:  name = "BTN_DPAD_DOWN"; break;
            case BTN_DPAD_LEFT:  name = "BTN_DPAD_LEFT"; break;
            case BTN_DPAD_RIGHT: name = "BTN_DPAD_RIGHT"; break;
            case BTN_TRIGGER_HAPPY1: name = "BTN_TRIGGER_HAPPY1(M1)"; break;
            case BTN_TRIGGER_HAPPY2: name = "BTN_TRIGGER_HAPPY2(M2)"; break;
            case BTN_TRIGGER_HAPPY3: name = "BTN_TRIGGER_HAPPY3"; break;
            case BTN_TRIGGER_HAPPY4: name = "BTN_TRIGGER_HAPPY4"; break;
            case KEY_PROG1: name = "KEY_PROG1(ArmouryCrate)"; break;
            case KEY_PROG2: name = "KEY_PROG2(CmdCenter)"; break;
            default: break;
        }
        fprintf(traceFile_, "%d %s\n", code, name);
        fflush(traceFile_); // flush every line so SSH tail works instantly
    }

    void LoadProfile() {
        // Try to load device profile. Path can be set via kernel cmdline
        // (playos.profile=) or defaults to the first profile found.
        const char* profileId = std::getenv("PLAYOS_PROFILE");
        std::string path;
        if (profileId && profileId[0]) {
            path = std::string("/etc/playos/device-profiles/") + profileId + ".toml";
        } else {
            // Auto-detect: look for any profile in the directory
            path = "/etc/playos/device-profiles/default.toml";
        }

        auto profile = DeviceProfile::Load(path);
        if (!profile) {
            // No profile found — use hardcoded defaults
            m_homeCodes = {BTN_MODE, BTN_TRIGGER_HAPPY1, BTN_TRIGGER_HAPPY2,
                           BTN_TRIGGER_HAPPY3, BTN_TRIGGER_HAPPY4, KEY_PROG1};
            m_qsCodes   = {KEY_PROG2};
            return;
        }

        // Resolve symbolic names → evdev codes via InputMapping
        if (!profile->input().homeButton.empty())
            m_homeCodes = Detail::ResolveInputCode(profile->input().homeButton);
        if (!profile->input().quickSettingsButton.empty())
            m_qsCodes   = Detail::ResolveInputCode(profile->input().quickSettingsButton);

        // Fallback: if symbolic name wasn't recognized, use hardcoded defaults
        if (m_homeCodes.empty())
            m_homeCodes = {BTN_MODE, BTN_TRIGGER_HAPPY1, BTN_TRIGGER_HAPPY2,
                           BTN_TRIGGER_HAPPY3, BTN_TRIGGER_HAPPY4, KEY_PROG1};
        if (m_qsCodes.empty())
            m_qsCodes = {KEY_PROG2};
    }

    int fd_ = -1;
    int homeFd_ = -1;   // separate device for Home/Guide button (ROG Ally etc.)
    int vendorFd_ = -1; // hid-asus-ally extra buttons (Crate, CC, M1/M2)
    FILE* traceFile_ = nullptr; // /tmp/playos-input.log for button tracing
    bool buttons_[static_cast<int>(Button::Count)] = {false};
    float axes_[6] = {0.0f};
    input_absinfo abs_[ABS_MAX + 1] = {};
    std::vector<int> m_homeCodes;
    std::vector<int> m_qsCodes;
};

} // namespace

IInputBackend* CreateInputBackend() {
    return new LinuxInputBackend();
}

} // namespace Detail
} // namespace PlayOS
