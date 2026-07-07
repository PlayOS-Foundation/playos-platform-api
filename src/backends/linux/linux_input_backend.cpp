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

#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstring>
#include <string>

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
    LinuxInputBackend() { openGamepad(); }
    ~LinuxInputBackend() override {
        if (fd_ >= 0) close(fd_);
    }

    void Update() override {
        if (fd_ < 0) return;
        input_event ev;
        ssize_t n;
        while ((n = read(fd_, &ev, sizeof(ev))) == (ssize_t)sizeof(ev)) {
            if (ev.type == EV_KEY) {
                onKey(ev.code, ev.value != 0);
            } else if (ev.type == EV_ABS) {
                onAbs(ev.code, ev.value);
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
            case BTN_MODE:   set(Button::Home, pressed); break;
            default: break;
        }
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
    float normStick(int code, int value) const {
        const auto& a = abs_[code];
        if (a.maximum == a.minimum) return 0.0f;
        const float t = (float)(value - a.minimum) / (float)(a.maximum - a.minimum);
        return t * 2.0f - 1.0f;
    }

    // Normalize a trigger axis to [0, 1].
    float normTrigger(int code, int value) const {
        const auto& a = abs_[code];
        if (a.maximum == a.minimum) return 0.0f;
        return (float)(value - a.minimum) / (float)(a.maximum - a.minimum);
    }

    void openGamepad() {
        DIR* dir = opendir("/dev/input");
        if (!dir) return;
        dirent* entry;
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

    int fd_ = -1;
    bool buttons_[static_cast<int>(Button::Count)] = {false};
    float axes_[6] = {0.0f};
    input_absinfo abs_[ABS_MAX + 1] = {};
};

} // namespace

IInputBackend* CreateInputBackend() {
    return new LinuxInputBackend();
}

} // namespace Detail
} // namespace PlayOS
